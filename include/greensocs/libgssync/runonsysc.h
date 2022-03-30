/*
* Copyright (c) 2022 GreenSocs
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version, or under the
* Apache License, Version 2.0 (the "License”) at your discretion.
*
* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
* You may obtain a copy of the Apache License at
* http://www.apache.org/licenses/LICENSE-2.0
*/

#ifndef RUNONSYSTEMC_H
#define RUNONSYSTEMC_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <future>

#include "greensocs/libgssync/async_event.h"

namespace gs {
    class RunOnSysC : public sc_core::sc_module {
    protected:
        class AsyncJob {
        public:
            using Ptr = std::shared_ptr<AsyncJob>;

        private:
            std::packaged_task<void ()> m_task;

            bool m_cancelled = false;

            void run_job()
            {
                m_task();
            }

        public:
            AsyncJob(std::function<void()> &&job)
                : m_task(job)
            {}

            AsyncJob(std::function<void()> &job)
                : m_task(job)
            {}

            AsyncJob() = delete;
            AsyncJob(const AsyncJob&) = delete;

            void operator()()
            {
                run_job();
            }

            /**
             * @brief Cancel a job
             *
             * @details Cancel a job by setting m_cancelled to true and by
             * resetting the task. Any waiter will then be unblocked immediately.
             */
            void cancel()
            {
                m_cancelled = true;  
                m_task.reset();
            }

            void wait()
            {
                auto future = m_task.get_future();

                future.wait();

                if (!m_cancelled) {
                    future.get();
                }
            }

            bool is_cancelled() const
            {
                return m_cancelled;
            }
        };

        std::thread::id m_thread_id;

        /* Async job queue */
        std::queue< AsyncJob::Ptr > m_async_jobs;
        AsyncJob::Ptr m_running_job;
        std::mutex m_async_jobs_mutex;

        async_event m_jobs_handler_event;

        // Process inside a thread incase the job calls wait
        void jobs_handler() {
            std::unique_lock<std::mutex> lock(m_async_jobs_mutex);
            for (;;) {
                while (!m_async_jobs.empty()) {
                    m_running_job = m_async_jobs.front();
                    m_async_jobs.pop();

                    lock.unlock();
                    sc_core::sc_unsuspendable(); // a wait in the job will cause systemc time to advance
                    (*m_running_job)();
                    sc_core::sc_suspendable();
                    lock.lock();

                    m_running_job.reset();
                }

                lock.unlock();
                wait(m_jobs_handler_event);
                lock.lock();
            }
        }

        void cancel_pendings_locked()
        {
            while (!m_async_jobs.empty()) {
                m_async_jobs.front()->cancel();
                m_async_jobs.pop();
            }
        }

    public:
        RunOnSysC(const sc_core::sc_module_name &n = sc_core::sc_module_name("run-on-sysc"))
            : sc_module(n)
            , m_thread_id(std::this_thread::get_id())
            , m_jobs_handler_event(false) // starve if no more jobs provided
        {
            SC_HAS_PROCESS(RunOnSysC);
            SC_THREAD(jobs_handler);
        }

        /**
         * @brief Cancel all pending jobs
         *
         * @detail Cancel all the pending jobs. The callers will be unblocked
         *         if they are waiting for the job.
         */
        void cancel_pendings()
        {
            std::lock_guard<std::mutex> lock(m_async_jobs_mutex);

            cancel_pendings_locked();
        }

        /**
         * @brief Cancel all pending and running jobs
         *
         * @detail Cancel all the pending jobs and the currently running job.
         *         The callers will be unblocked if they are waiting for the
         *         job. Note that if the currently running job is resumed, the
         *         behaviour is undefined. This method is meant to be called
         *         after simulation has ended.
         */
        void cancel_all()
        {
            std::lock_guard<std::mutex> lock(m_async_jobs_mutex);

            cancel_pendings_locked();

            if (m_running_job) {
                m_running_job->cancel();
                m_running_job.reset();
            }
        }

        void end_of_simulation()
        {
            cancel_all();
        }

        void fork_on_systemc(std::function<void()> job_entry)
        {
            run_on_sysc(job_entry, false);
        }

        /**
         * @brief Run a job on the SystemC kernel thread
         *
         * @param[in] job_entry The job to run
         * @param[in] wait If true, wait for job completion
         *
         * @return true if the job has been succesfully executed or if `wait`
         *         was false, false if it has been cancelled (see
         *         `RunOnSysC::cancel_all`).
         */
        bool run_on_sysc(std::function<void()> job_entry, bool wait = true)
        {
            if (std::this_thread::get_id() == m_thread_id) {
                job_entry();
                return true;
            } else {
                AsyncJob::Ptr job(new AsyncJob(job_entry));

                {
                    std::lock_guard<std::mutex> lock(m_async_jobs_mutex);
                    m_async_jobs.push(job);
                }

                m_jobs_handler_event.async_notify();

                if (wait) {
                    /* Wait for job completion */
                    job->wait();
                    return !job->is_cancelled();
                }

                return true;
            }
        }
    };
}
#endif // RUNONSYSTEMC_H

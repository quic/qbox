/*
 *  Copyright (C) 2020  GreenSocs
 *
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
        /* Job that are created by the async worker, meant to be executed by the
         * kernel.
         */
        std::thread::id m_thread_id;
        
        class AsyncJob {
        private:
            std::packaged_task<void()> m_job;
            
        public:
            AsyncJob(std::function<void()> &&job) : m_job(std::move(job)), accepted(false), cancelled(false) {}
            AsyncJob(std::function<void()> &job) : m_job(job), accepted(false), cancelled(false) {}
            AsyncJob() = delete;
            AsyncJob(const AsyncJob&) = delete;

            std::packaged_task<void()> & get_job() { return m_job; }
            
            void operator()() { m_job(); }

            bool accepted;
            bool cancelled;
        };
        
        /* Async job queue */
        std::queue< std::shared_ptr<AsyncJob> > async_jobs;
        std::mutex mutex;
        
        async_event jobsHandlerEvent;
        
        // Process inside a thread incase the job calls wait
        void jobsHandler() {
            std::unique_lock<std::mutex> lock(mutex);
            for (;;) {
                while (!async_jobs.empty()) {
                    std::shared_ptr<AsyncJob> job = async_jobs.front();
                    async_jobs.pop();
                    job->accepted=true;
                    
                    lock.unlock();
                    sc_core::sc_unsuspendable(); // a wait in the job will cause systemc time to advance
                    (*job)();
                    sc_core::sc_suspendable();
                    lock.lock();
                }
                
                lock.unlock();
                wait(jobsHandlerEvent);
                lock.lock();
            }
        }
        
    public:
        RunOnSysC() : sc_module(sc_core::sc_module_name("RunOnSysC"))
                    , m_thread_id(std::this_thread::get_id())
                    , jobsHandlerEvent(false) // starve if no more jobs provided
        {
            SC_HAS_PROCESS(RunOnSysC);
            SC_THREAD(jobsHandler);
        }

        void end_of_simulation() {
            {
                std::lock_guard<std::mutex> lock(mutex);
                while (!async_jobs.empty()) {
                    std::shared_ptr<AsyncJob> job = async_jobs.front();
                    async_jobs.pop();
                    (*job).cancelled = true;
                }
            }
        }
        
        void fork_on_systemc(std::function<void()> job_entry) 
        {
            run_on_sysc(job_entry, false);
        }
                
        void run_on_sysc(std::function<void()> job_entry, bool complete=true)
        {
            if (std::this_thread::get_id() == m_thread_id) {
                job_entry();
            } else {
                std::shared_ptr<AsyncJob> job(new AsyncJob(job_entry));
                
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    async_jobs.push(job);
                }
                
                jobsHandlerEvent.async_notify();
                
                
                if (complete) {
                    /* Wait for job completion */
                    std::chrono::system_clock::time_point one_second
                            = std::chrono::system_clock::now() + std::chrono::seconds(1);
                    auto result = job->get_job().get_future();
                    while (result.wait_until(one_second) != std::future_status::ready) {
                        if(job->cancelled)
                            return;
                    }
                    result.get();
                }
            }
        }

    };
}
#endif // RUNONSYSTEMC_H

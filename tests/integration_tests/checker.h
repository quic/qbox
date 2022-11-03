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

#include <iostream>
#include <chrono>
#include <systemc>
#include <cci_configuration>
#include <thread>
#include <mutex>
#include <iomanip>
#include <deque>
#include <condition_variable>
#include <fstream>
#include <queue>
#include "greensocs/libgssync.h"

class Checker
{
public:
    enum eventTypes {
        TimeStamp,
        Budget,
        ClockTick,
        TxnSent,
        TxnRec,
        Finished,
        StartTest,
        StopTest,
        SetQuantum,
        FinishedInstructions,
    };
    const char* evTypeString(eventTypes t) {
        switch (t) {
        case TimeStamp:
            return "Time Stamp";
        case ClockTick:
            return "Clock tick";
        case Budget:
            return "Budget";
        case TxnSent:
            return "Txn Sent";
        case TxnRec:
            return "Txn Recieved";
        case Finished:
            return "Model Finished";
        case StopTest:
            return "Stop Test";
        case StartTest:
            return "Start Test";
        case SetQuantum:
            return "n/a";
        case FinishedInstructions:
            return "n/a";
        default:
            return "unexpected event type";
        }
    }

private:
    std::thread m_wd_thread;
    std::thread m_analyse_thread;
    std::thread m_process_thread;
    bool wd_ok;
    bool wd_on;
    int verbose;
    int local_verbose;

    int testsDone;
    bool finished;
    std::condition_variable analysiscond;

    struct Event {
        enum eventTypes event;
        sc_core::sc_object* model;
        std::chrono::high_resolution_clock::time_point realtime;
        sc_core::sc_time simtime;
        sc_core::sc_time time;

        bool operator==(const Event& e) const { return event == e.event && model == e.model; }
    };

    class EventQueue
    {
        static const int max = 5000;
        static const int min = 4500;
        Event array[max];
        int in = 0, out = 0;
        std::atomic_int size;
        std::mutex mutex;
        std::condition_variable cond;

    public:
        EventQueue(): size(0) {}

        std::chrono::high_resolution_clock::duration push(Event& e) {
            auto delay = std::chrono::high_resolution_clock::duration::zero();
            if (size >= max - 1) {
                // only calculate times if we wait
                auto pausetime = std::chrono::high_resolution_clock::now();
                std::unique_lock<std::mutex> lock(mutex);
                cond.wait(lock, [this] { return (size < min); });
                delay = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::high_resolution_clock::now() - pausetime);
            }
            array[in] = e;
            in = (in + 1) % max;
            size++;
            cond.notify_all();
            return delay;
        }
        /* NB only one reading thread */
        bool try_pop(Event& e) {
            if (size == 0)
                return false;
            e = array[out];
            out = (out + 1) % max;
            size--;
            cond.notify_all();
            return true;
        }
        void wait_pop(Event& e) {
            if (size == 0) { // we might accidently think there is nothing in the list.
                // significant performance improvement if we normally dont hit
                // the cond.wait
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                std::unique_lock<std::mutex> lock(mutex);
                cond.wait(lock, [this] { return (size != 0); });
            }
            e = array[out];
            out = (out + 1) % max;
            size--;
            cond.notify_all();
        }
        size_t empty() { return size == 0; }
    };

    std::string to_string(const Event& e) { return evTypeString(e.event); }

    typedef std::deque<Event> eventList;
    eventList events;
    EventQueue eventQueue;
    std::queue<std::pair<std::string, std::vector<eventList>>> allevents;

    bool recording;
    std::string currentTest;
    sc_core::sc_time startTime;

    std::mutex mutex;
    std::mutex record_mutex;

    std::chrono::high_resolution_clock::duration realtimeOffset;
    std::chrono::high_resolution_clock::time_point realtimeStart;

    cci::cci_broker_handle m_broker;

public:
    gs::async_event stopEvent;

    void printevent(Event e, bool rec) {
        sc_core::sc_time time = e.time;
        const char* timename;
        switch (e.event) {
        case TxnSent:
            timename = " Delta time:";
            break;
        case TxnRec:
            timename = " Delta time:";
            break;
        case TimeStamp:
            timename = " Local time:";
            time -= startTime;
            break;
        case Budget:
            timename = " run Budget :";
            break;
        case SetQuantum:
            timename = " Quantum :";
            break;
        default:
            timename = "         n/a :";
        }

        SCP_INFO("checker.h") << std::chrono::duration_cast<std::chrono::microseconds>(
                                     e.realtime - realtimeStart)
                                     .count()
                              << " sc_time:" << e.simtime - startTime << timename << time << " "
                              << ((e.model) ? e.model->name() : "n/a")
                              << " event: " << evTypeString(e.event) << ((!rec) ? "(!R)" : " ");
    }

    void record(sc_core::sc_module* model, enum eventTypes eventType,
                sc_core::sc_time time = sc_core::SC_ZERO_TIME) {
        std::chrono::high_resolution_clock::time_point
            realtime = std::chrono::high_resolution_clock::now();
        realtime -= realtimeOffset;
        sc_core::sc_time simtime = sc_core::sc_time_stamp();

        Event r{ eventType, model, realtime, simtime, time };

        std::lock_guard<std::mutex> lock(record_mutex); // gate keeper

        if (eventType == StartTest) {
            recording = true;
        }

        if (recording) {
            realtimeOffset += eventQueue.push(r);
        } else {
            if (verbose > 1 && local_verbose > 1) {
                printevent(r, false);
            }
        }
        if (eventType == StopTest) {
            recording = false;
            sleep_cond.notify_all();
        }
    }
    void processevents() {
        std::chrono::high_resolution_clock::time_point stoptime;

        for (;;) {
            Event e;

            eventQueue.wait_pop(e);

            wd_ok = true;

            events.push_back(std::move(e));

            if (verbose > 1 && local_verbose > 1) {
                printevent(e, true);
            }

            if (e.event == StopTest) {
                stoptime = e.realtime;
                break;
            }
        }
        for (Event e; eventQueue.try_pop(e);) {
            events.push_back(std::move(e));
        }
        wd_ok = true;
    }

    void startRecording(std::string name) {
        const std::lock_guard<std::mutex> lock(mutex);

        for (Event e; eventQueue.try_pop(e);) {
        }

        assert(recording == false);
        assert(eventQueue.empty());

        currentTest = name;
        recording = true;

        startTime = sc_core::sc_time_stamp();

        cci::cci_value v = m_broker.get_preset_cci_value(name + ".verbose");
        if (v.is_int()) {
            local_verbose = v.get_int();
        } else {
            local_verbose = verbose;
        }

        //        if (verbose>1) {
        //            std::cout << "Starting test "<<name<<"\n";
        //        }

        m_process_thread = std::thread(&Checker::processevents, this);
    }
    void stopRecording() {
        recording = false;

        wd_ok = true;
        m_process_thread.join();
        //        assert(eventQueue.size_approx()==0);

        wd_ok = true;
        std::unique_lock<std::mutex> lock(mutex);

        eventList tmp = std::move(events);

        if (allevents.empty() || allevents.back().first != currentTest) {
            std::vector<eventList> el;
            allevents.push(std::make_pair(currentTest, std::move(el)));
        }
        allevents.back().second.push_back(tmp);
    }

    void finishedTest() {
        const std::lock_guard<std::mutex> lock(mutex);
        testsDone++;
        analysiscond.notify_all();
    }
    void allDone() {
        const std::lock_guard<std::mutex> lock(mutex);
        finished = true;
        wd_on = false;
        analysiscond.notify_all();
    }

    float to_us(sc_core::sc_time t) { return t.to_seconds() * 1000000; }

    void analyse() {
        if (verbose == 0)
            return;
        std::unique_lock<std::mutex> lock(mutex);

        std::stringstream csv;

        SCP_INFO("checker.h") << "\n\n\n Analysis of runs \n";
        SCP_INFO("checker.h") << std::setprecision(5) << std::fixed;

        csv << "test, sc_time, ins bound, Deterministic, backwards, sc ahead, realtime, run "
               "budget, quantum, error, worst, txns, dropped, desc\n";

        for (;;) {
            analysiscond.wait(lock, [this] { return (finished || testsDone > 0); });
            if (finished && testsDone == 0)
                break;
            assert(!allevents.empty());
            auto test_p = allevents.front();
            allevents.pop();
            testsDone--;
            wd_ok = true;

            SCP_INFO("checker.h") << "\nTest : " << test_p.first;
            cci::cci_value desc = m_broker.get_preset_cci_value(test_p.first + ".description");
            if (desc.is_string()) {
                SCP_INFO("checker.h") << desc.get_string();
            }
            csv << test_p.first << ",";
            /* simulation time */
            {
                sc_core::sc_time runtime = sc_core::SC_ZERO_TIME;
                int ibound = 0;
                int iboundi = 0;
                int i = 0;
                for (auto run : test_p.second) {
                    bool iboundfound = false;
                    sc_core::sc_time start = sc_core::SC_ZERO_TIME;
                    sc_core::sc_time stop = sc_core::SC_ZERO_TIME;

                    for (auto evt : run) {
                        if (evt.event == StartTest) {
                            start = evt.simtime;
                        }
                        // take earliest stop point
                        if (stop == sc_core::SC_ZERO_TIME && evt.event == StopTest) {
                            stop = evt.simtime;
                        }
                        if (stop == sc_core::SC_ZERO_TIME && evt.event == Finished) {
                            stop = evt.simtime;
                        }

                        if (evt.event == FinishedInstructions)
                            iboundfound = true;
                    }
                    if ((start != sc_core::SC_ZERO_TIME) && (stop != sc_core::SC_ZERO_TIME)) {
                        runtime += stop - start;
                        i++;
                    }
                    if (iboundfound)
                        ibound++;
                    iboundi++;
                }
                SCP_INFO("checker.h")
                    << "  Average Simulation (sc_)time " << to_us(runtime) / (float)i << "us";
                if (!ibound)
                    SCP_INFO("checker.h") << " Time bound";
                else if (ibound == iboundi) {
                    SCP_INFO("checker.h") << "  Instruction bound";
                } else {
                    SCP_INFO("checker.h")
                        << "  " << ibound * 100 / iboundi << "% runs instruction bound";
                }
                csv << to_us(runtime) / (float)i << ",";
                csv << ibound * 100 / iboundi << ",";
            }

            /* is it determanistic */
            {
                if (test_p.second.size() >= 2) {
                    auto run_it = test_p.second.begin();
                    eventList one = *run_it;
                    bool result = true;
                    int i = 2;
                    for (++run_it; run_it != test_p.second.end(); ++run_it, ++i) {
                        eventList run = *run_it;

                        if (!std::equal(one.begin(), one.end(), run.begin())) {
                            std::equal(one.begin(), one.end(), run.begin(),
                                       [this, i](auto a, auto b) -> bool {
                                           if (!(a == b)) {
                                               SCP_INFO("checker.h")
                                                   << "  Non determinitic Difference in run " << i
                                                   << " between " << a.model->name() << " (at time "
                                                   << std::chrono::duration_cast<
                                                          std::chrono::microseconds>(a.realtime -
                                                                                     realtimeStart)
                                                          .count()
                                                   << ")"
                                                   << " and " << b.model->name() << " (at time "
                                                   << std::chrono::duration_cast<
                                                          std::chrono::microseconds>(b.realtime -
                                                                                     realtimeStart)
                                                          .count()
                                                   << ")"

                                                   << " " << to_string(a) << " != " << to_string(b);
                                               return false;
                                           }
                                           return true;
                                       });
                            result = false;
                            break;
                        }
                    }
                    if (result == false) {
                        SCP_INFO("checker.h") << "  Non determanistic";
                    } else {
                        if (test_p.second.size() > 5) {
                            if (result)
                                std::cout << "  Determanistic\n";
                        } else {
                            SCP_INFO("checker.h") << "  Not enough data to determin determanism";
                        }
                    }
                    csv << result << ",";
                } else {
                    csv << "0,";
                }
            }

            /* check for incrementing clocks when txn recieved */
            {
                int txns = 0;
                int time_backwards = 0;
                for (auto run : test_p.second) {
                    std::map<sc_core::sc_object*, sc_core::sc_time> last_times;
                    for (auto evt : run) {
                        if (evt.event == TxnRec) {
                            if (last_times.find(evt.model) == last_times.end())
                                last_times[evt.model] = sc_core::SC_ZERO_TIME;
                            if ((evt.simtime + evt.time) < last_times[evt.model]) {
                                time_backwards++;
                            }
                            last_times[evt.model] = (evt.simtime + evt.time);
                        }
                    }
                }
                if (time_backwards) {
                    SCP_INFO("checker.h")
                        << "Time went backwards for a target " << time_backwards << " times";
                }
                csv << time_backwards << ",";
            }

            /* Check for SystemC behind */
            {
                int ahead = 0;
                for (auto run : test_p.second) {
                    for (auto evt : run) {
                        if (evt.event == TimeStamp) {
                            if (evt.simtime > evt.time)
                                ahead++;
                        }
                    }
                }
                if (ahead) {
                    SCP_INFO("checker.h") << "  SystemC ahead " << ahead << " times";
                }
                csv << ahead << ",";
            }

            /* check execution time */
            {
                int n = 0;
                auto duration_total = 0;
                for (auto run : test_p.second) {
                    n++;
                    std::chrono::high_resolution_clock::time_point start, end;
                    start = run.front().realtime;
                    end = run.back().realtime;
                    duration_total += std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                                            start)
                                          .count();
                }
                duration_total /= (float)n;
                SCP_INFO("checker.h") << "  Average execution time: " << duration_total << "ms";
                csv << duration_total << ",";
            }

            /* Average run budget */
            {
                int n = 0;
                sc_core::sc_time total_Budget = sc_core::SC_ZERO_TIME;
                for (auto run : test_p.second) {
                    for (auto evt : run) {
                        if (evt.event == Budget) {
                            total_Budget += evt.time;
                            n++;
                        }
                    }
                }
                if (n) {
                    SCP_INFO("checker.h")
                        << "  Average run budget " << to_us(total_Budget / (float)n) << "us";
                }
                csv << (n ? to_us(total_Budget / (float)n) : 0) << ",";
            }

            /* Average run skew */
            {
                int n = 0;
                sc_core::sc_time total_error = sc_core::SC_ZERO_TIME;
                sc_core::sc_time worst = sc_core::SC_ZERO_TIME;
                sc_core::sc_time quantum;
                for (auto run : test_p.second) {
                    for (auto evt : run) {
                        if (evt.event == SetQuantum) {
                            quantum = evt.time;
                        }
                        if (evt.event == TimeStamp) {
                            sc_core::sc_time err = (evt.simtime > evt.time)
                                                       ? evt.simtime - evt.time
                                                       : evt.time - evt.simtime;
                            total_error += err;
                            if (err > worst)
                                worst = err;
                            n++;
                        }
                    }
                }
                if (n) {
                    SCP_INFO("checker.h") << "  Quantum " << to_us(quantum) << "us";
                    SCP_INFO("checker.h")
                        << "  Average error " << to_us(total_error / (float)n) << "us";
                    SCP_INFO("checker.h") << "  worst error " << to_us(worst) << "us";
                    csv << to_us(quantum) << "," << to_us(total_error / (float)n) << ","
                        << to_us(worst) << ",";
                } else {
                    csv << "0,0,0,";
                }
            }

            /* Average txns */
            {
                int n = 0;
                int rec = 0;
                int sent = 0;
                for (auto run : test_p.second) {
                    n++;
                    for (auto evt : run) {
                        if (evt.event == TxnSent) {
                            sent++;
                        }
                        if (evt.event == TxnRec) {
                            rec++;
                        }
                    }
                }
                if (n && sent) {
                    SCP_INFO("checker.h") << "  Average txns " << (float)sent / (float)n;
                    if (rec != sent) {
                        SCP_INFO("checker.h") << " Total transactions dropped (!) : " << sent - rec;
                    }
                }
                csv << (n ? (float)sent / (float)n : 0) << "," << sent - rec << ",";
            }
            if (desc.is_string()) {
                csv << "\"" << desc.get_string() << "\"";
            } else {
                csv << "\" \"";
            }
            csv << "\n";
        }

        auto uncon = m_broker.get_unconsumed_preset_values(
            [](const std::pair<std::string, cci::cci_value>& iv) {
                return (iv.first.find("tests.") != std::string::npos &&
                        iv.first.find(".verbose") == std::string::npos);
            });
        for (auto v : uncon) {
            SCP_INFO("checker.h") << "Unconsumed config value: " << v.first;
        }

        if (m_broker.get_preset_cci_value("csvfile").is_string()) {
            std::string csvfilename = m_broker.get_preset_cci_value("csvfile").get_string();
            std::ofstream csvfile;
            csvfile.open(csvfilename, std::ios::out | std::ios::trunc);
            csvfile << csv.str();
            csvfile.close();
        }
    }

    Checker()
        : recording(false)
        , currentTest("none")
        , events()
        , allevents()
        , wd_ok(true)
        , testsDone(0)
        , finished(false)
        , startTime(sc_core::SC_ZERO_TIME)
        , m_broker(cci::cci_get_global_broker(cci::cci_originator("gs_checker")))
        , stopEvent(false) // starve if no more events
    {
        realtimeStart = std::chrono::high_resolution_clock::now();
        realtimeOffset = std::chrono::high_resolution_clock::duration::zero();

        verbose = m_broker.get_preset_cci_value("verbose").get_int();

        if (verbose != 0) {
            SCP_INFO("checker.h") << "Verbosity : " << verbose;
        }

        wd_on = true;
        m_wd_thread = std::thread(&Checker::watchdog, this);
        m_analyse_thread = std::thread(&Checker::analyse, this);
    }

    ~Checker() {
        m_wd_thread.join();
        m_analyse_thread.join();
    }

    void watchdog() {
        while (wd_on) {
            wd_ok = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            if (!wd_on)
                break;
            assert(wd_ok == true);
        }
    }

private:
    std::mutex sleep_mutex;
    std::condition_variable sleep_cond;

public:
    template <class Rep, class Period>
    void sleep_for(const std::chrono::duration<Rep, Period>& rel_time) {
        std::unique_lock<std::mutex> lock(sleep_mutex);
        sleep_cond.wait_for(lock, rel_time, [this] { return !recording; });
    }
};

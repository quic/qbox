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

#ifdef _WIN32
#include <windows.h>
#endif

#include <iostream>
#include <chrono>
#include <list>
#include <random>
#include <typeinfo>

#ifndef SC_INCLUDE_DYNAMIC_PROCESSES
#define SC_INCLUDE_DYNAMIC_PROCESSES
#endif
#include <systemc>
#include <tlm>
#include <cci_configuration>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/tlm_quantumkeeper.h>
#include <cci/utils/broker.h>
#include <greensocs/libgsutils.h>

#include "checker.h"
#include "router.h"

using namespace sc_core;

namespace gs {
class ModuleFactory
{
    template <class... Args>
    struct MapHolder {
        static std::map<std::string,
                        std::function<sc_core::sc_module*(sc_core::sc_module_name, Args...)>>&
        get() {
            static std::map<std::string,
                            std::function<sc_core::sc_module*(sc_core::sc_module_name, Args...)>>
                m_map;
            return m_map;
        }
    };

public:
    template <class... Args>
    static bool Reg(std::string name,
                    std::function<sc_core::sc_module*(sc_core::sc_module_name, Args...)> Callback) {
        MapHolder<Args...>::get()[name] = Callback;
        return true;
    }

    template <class... Args>
    static sc_core::sc_module* Create(const std::string& moduletype, sc_core::sc_module_name name,
                                      Args&&... args) {
        auto map = MapHolder<Args...>::get();
        if (map.find(moduletype) != map.end()) {
            return MapHolder<Args...>::get()[moduletype](name, std::forward<Args>(args)...);
        } else {
            return nullptr;
        }
    }
};

#define GSC_MODULE_REGISTER(__name__)                                               \
    bool __name__##_gs_module_reg_entry = gs::ModuleFactory::Reg(                   \
        #__name__, (std::function<sc_core::sc_module*(sc_core::sc_module_name)>)[]( \
                       sc_core::sc_module_name _instname)                           \
                           ->sc_core::sc_module *                                   \
                       { return new __name__(_instname); })

#define GSC_MODULE_REGISTER_1(__name__, __T1__)                                             \
    bool __name__##_gs_module_reg_entry = gs::ModuleFactory::Reg(                           \
        #__name__, (std::function<sc_core::sc_module*(sc_core::sc_module_name, __T1__)>)[]( \
                       sc_core::sc_module_name _instname, __T1__ _t1)                       \
                           ->sc_core::sc_module *                                           \
                       { return new __name__(_instname, _t1); });

} // namespace gs

/**********************
 * TLM Convenience
 **********************/
SC_MODULE (Model) {
    Checker& checker;

public:
    cci::cci_param<std::string> moduletype;
    virtual void entry(){};
    virtual void stop(){};
    virtual void exit(){};
    Model(sc_module_name _name, Checker & _checker)
        : sc_module(_name)
        , checker(_checker)
        , moduletype("moduletype", "none", "Lua module type name") // this is unused, but
                                                                   // makes sure the param
                                                                   // is used.
    {}
};

class Initiator : public Model
{
public:
    tlm_utils::simple_initiator_socket<Initiator> initiator_socket;
    Initiator(sc_module_name _name, Checker& _checker)
        : Model(_name, _checker), initiator_socket("initiatorSocket") {
        SCP_DEBUG(SCMOD) << "In the constructor Initiator";
    }
};
class Target : public Model
{
public:
    cci::cci_param<uint64_t> add_base;
    cci::cci_param<uint64_t> add_size;
    virtual void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay) = 0;
    tlm_utils::simple_target_socket<Target> target_socket;
    virtual bool requires_systemc() = 0;
    Target(sc_module_name _name, Checker& _checker)
        : Model(_name, _checker)
        , add_base("add_base", 0)
        , add_size("add_size", 1)
        , target_socket("targetSocket") {
        target_socket.register_b_transport(this, &Target::b_transport);
        SCP_DEBUG(SCMOD) << "In the constructor Target";
    }
};

/**********************
 * Models
 **********************/

class QemuLikeMaster : public Initiator
{
public:
    cci::cci_param<int> run_count;
    cci::cci_param<int> send_to;
    cci::cci_param<std::string> sync_policy;
    cci::cci_param<int> MIPS;
    cci::cci_param<int> iorate;
    cci::cci_param<int> ms_sleep;

private:
    gs::tlm_quantumkeeper_extended* qk;
    gs::RunOnSysC m_on_sysc;

    bool running;
    bool active;
    std::thread m_worker_thread;
    bool m_worker_thread_active;

    bool run_via_qk;

    void run() {
        active = true;
        qk->reset();

        qk->sync();

        sc_time nexttxn = qk->get_current_time();
        uint64_t icount;
        for (icount = 0; running && (run_count == -1 || icount < run_count);) {
            sc_time budget = qk->time_to_sync();
            checker.record(this, Checker::Budget, budget);

            // this loop intentionally not efficient, represents QEMU running
            // some instructions
            for (uint64_t starti = icount;
                 running && icount < run_count &&
                 (icount - starti) < (budget.to_seconds() * (float)MIPS * 1000000.0);
                 icount++) {
                qk->inc(sc_time(1.0 / (float)MIPS, SC_US));
                // we will not check for sync on each instruction!
                if (qk->get_current_time() >= nexttxn) {
                    nexttxn += sc_time(iorate / (float)MIPS, SC_US);
                    ;

                    // account for the time between IO events
                    if (ms_sleep) {
                        checker.sleep_for(std::chrono::milliseconds(ms_sleep));
                    }
                    if (qk->need_sync()) {
                        qk->sync();
                        if (!running)
                            break;
                    }
                    /* send transaction */
                    tlm::tlm_generic_payload trans;
                    trans.set_address(send_to);

                    checker.record(this, Checker::TimeStamp, qk->get_current_time());

                    /*
                      sc_time local_drift, local_drift_before;
                    //                    m_sync->send_txn([this, &trans, &local_drift,
                    //                    &local_drift_before] {

                                        // this shoudl eitherbe run on a run_on_systemc, now, or
                                        // delayed, OR we mustbe sure that the end model doesn't
                                        // care

                                                             sc_time
                    current=sc_core::sc_time_stamp(); sc_time l_time=qk->get_current_time();
                                                             // NB if sc_time drifts during the

                                                             if (l_time > current) {
                                                                 local_drift=l_time - current;
                                                             } else {
                                                                 local_drift=SC_ZERO_TIME;
                                                             }
                                                             local_drift_before=local_drift;
                                                             checker.record(this,Checker::TxnSent,
                    local_drift);

                                                             initiator_socket->b_transport(trans,
                    local_drift);
                    */

                    m_on_sysc.run_on_sysc([this, &trans] {
                        if (qk->get_current_time() < sc_time_stamp()) {
                            sc_time delay = SC_ZERO_TIME;
                            checker.record(this, Checker::TxnSent, delay);
                            initiator_socket->b_transport(trans, delay);
                            qk->inc(delay);
                        } else {
                            sc_time delay = qk->get_local_time();
                            checker.record(this, Checker::TxnSent, delay);
                            initiator_socket->b_transport(trans, delay);
                            qk->set(delay);
                        }
                    });

                    if (running && qk->need_sync())
                        qk->sync();

#if 0
                                         
                     if (local_drift > local_drift_before) {
                        /* Account for time passed during the transaction 
                        * this isn't very accurate, They could have done a
                        * wait(local_drift), then added to local_drift. We
                        * should also add that offset here. Equally, they could
                        * wait(local_drift) then add an offset greater that
                        * local_drift_before - which we would wrongly interpret.
                        * Perhaps we could assert that the local_drift is either
                        * GT, or zero. But thats not what TLM seems to say...*/

                        qk->inc(local_drift - local_drift_before);
                        if (qk->need_sync()) {
                            qk->sync();
                        }
                    }
#endif
                }
            }

            if (running && qk->need_sync()) {
                qk->sync();
            }
        }

        checker.stopEvent.notify(SC_ZERO_TIME);
        if (icount >= run_count) {
            checker.record(this, Checker::FinishedInstructions);
        }
        running = false;
        checker.record(this, Checker::Finished);
        active = false;
    }

public:
    QemuLikeMaster(sc_module_name _name, Checker& _checker)
        : Initiator(_name, _checker)
        , run_count("run_count", 1000)
        , send_to("send_to", 0)
        , sync_policy("sync_policy", "tlm2")
        , MIPS("MIPS", 10)
        , iorate("iorate", 1000)
        , ms_sleep("ms_sleep", 0)
        , running(false)
        , active(false)
        , m_worker_thread_active(false) {
        SCP_DEBUG(SCMOD) << "In the constructor QemuLikeMaster";
        SC_HAS_PROCESS(QemuLikeMaster);
        SC_METHOD(stop);
        dont_initialize();
        sensitive << checker.stopEvent;

        if ((std::string)sync_policy == "tlm2")
            qk = new gs::tlm_quantumkeeper_extended();
        else if ((std::string)sync_policy == "multithread")
            qk = new gs::tlm_quantumkeeper_multithread();
        else if ((std::string)sync_policy == "multithread-quantum")
            qk = new gs::tlm_quantumkeeper_multi_quantum();
        else if ((std::string)sync_policy == "multithread-tlm2")
            qk = new gs::tlm_quantumkeeper_multithread();
        else
            std::abort(); // Disabled libssync tests
    }
    void entry() {
        running = true;
        qk->reset();
        wait(SC_ZERO_TIME); // Allow slaves to initilize

        if ((std::string)sync_policy == "multithread-tlm2") {
            qk->start();
            run();
        } else if ((std::string)sync_policy == "tlm2") {
            qk->start();
            run();
        } else if ((std::string)sync_policy == "multithread") {
            qk->start();
            std::thread t(std::bind(&QemuLikeMaster::run, this));
            m_worker_thread = std::move(t);
            m_worker_thread_active = true;
        } else if ((std::string)sync_policy == "multithread-quantum") {
            qk->start();
            std::thread t(std::bind(&QemuLikeMaster::run, this));
            m_worker_thread = std::move(t);
            m_worker_thread_active = true;
        } else {
            qk->start(std::bind(&QemuLikeMaster::run, this));
        }
    }
    void stop() {
        running = false;
        qk->stop();
    }
    void exit() {
        while (active) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            qk->stop(); // some QK's dont stop cleanly
        }

        if (m_worker_thread_active)
            m_worker_thread.join();
        m_worker_thread_active = false;
    }
};
GSC_MODULE_REGISTER_1(QemuLikeMaster, Checker&);

class MasterSimple : public Initiator
{
public:
    cci::cci_param<int> run_count;
    cci::cci_param<int> send_to;
    cci::cci_param<int> ns_wait;
    cci::cci_param<int> ms_sleep;
    cci::cci_param<int> ns_per_cycle;

private:
    bool running;

public:
    MasterSimple(sc_module_name _name, Checker& _checker)
        : Initiator(_name, _checker)
        , run_count("run_count", 1000)
        , send_to("send_to", 0)
        , ns_wait("ns_wait", 0)
        , ms_sleep("ms_sleep", 0)
        , ns_per_cycle("ns_per_cycle", 10)
        , running(false) {
        SCP_DEBUG(SCMOD) << "In the constructor MasterSimple";
        SC_HAS_PROCESS(MasterSimple);
        SC_METHOD(stop);
        dont_initialize();
        sensitive << checker.stopEvent;
    }
    void entry() {
        running = true;
        for (int i = 0; running && (run_count == -1 || i < run_count); i++) {
            wait(sc_time(ns_wait, SC_NS), checker.stopEvent);
            if (!running)
                break;
            if (ms_sleep) {
                checker.sleep_for(std::chrono::milliseconds(ms_sleep));
            }

            sc_core::sc_time local_drift = sc_core::SC_ZERO_TIME;
            checker.record(this, Checker::TimeStamp, sc_core::sc_time_stamp());

            checker.record(this, Checker::TxnSent, local_drift);
            /* send transaction */
            tlm::tlm_generic_payload trans;
            trans.set_address(send_to);
            initiator_socket->b_transport(trans, local_drift);

            wait(local_drift, checker.stopEvent);
        }
        checker.record(this, Checker::Finished);
        checker.stopEvent.notify(SC_ZERO_TIME);
    }
    void stop() { running = false; }
    void exit() {}
};
GSC_MODULE_REGISTER_1(MasterSimple, Checker&);

class Slave : public Target
{
public:
    cci::cci_param<int> ns_wait;
    cci::cci_param<int> ms_sleep;

private:
    virtual void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay) {
        checker.record(this, Checker::TxnRec, delay);

        if (ns_wait) {
            wait(sc_time(ns_wait, SC_NS), checker.stopEvent);
        }

        if (ms_sleep) {
            checker.sleep_for(std::chrono::milliseconds(ms_sleep));
        }
    }

public:
    Slave(sc_module_name _name, Checker& _checker)
        : Target(_name, _checker), ns_wait("ns_wait", 0), ms_sleep("ms_sleep", 0) {
        SCP_DEBUG(SCMOD) << "In the constructor Slave";
    }
    bool requires_systemc() { return (ns_wait); }
};
GSC_MODULE_REGISTER_1(Slave, Checker&);

class TLMClockedSlave : public Target
{
public:
    cci::cci_param<bool> tlm_mode;
    cci::cci_param<int> ns_wait;
    cci::cci_param<int> ns_delay;
    cci::cci_param<int> ms_sleep;

private:
    bool running;
    sc_event triggure;
    virtual void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay) {
        checker.record(this, Checker::TxnRec, delay);
        if (!running)
            return;
        if (ns_wait) {
            if (tlm_mode) {
                wait(delay, checker.stopEvent);
                delay = SC_ZERO_TIME;
            }
            if (running)
                wait(triggure | checker.stopEvent);
            checker.record(this, Checker::ClockTick);
        }
        if (ns_delay)
            delay += sc_time(ns_delay, SC_NS);

        if (ms_sleep && running) {
            checker.sleep_for(std::chrono::milliseconds(ms_sleep));
        }
    }

public:
    TLMClockedSlave(sc_module_name _name, Checker& _checker)
        : Target(_name, _checker)
        , tlm_mode("tlm_mode", true)
        , ns_wait("ns_wait", 0)
        , ns_delay("ns_delay", 0)
        , ms_sleep("ms_sleep", 0)
        , running(false) {
        SCP_DEBUG(SCMOD) << "In the constructor TLMClockedSlave";
        SC_HAS_PROCESS(TLMClockedSlave);
        SC_METHOD(stop);
        dont_initialize();
        sensitive << checker.stopEvent;
    }

    void entry() {
        if (ns_wait) {
            running = true;
            while (running) {
                wait(sc_core::sc_time(ns_wait, SC_NS), checker.stopEvent);
                triggure.notify(SC_ZERO_TIME);
            }
            checker.record(this, Checker::Finished);
        } else {
            running = false;
            triggure.notify(SC_ZERO_TIME);
        }
    }
    void stop() { running = false; }

    bool requires_systemc() { return (ns_wait != 0); }
};
GSC_MODULE_REGISTER_1(TLMClockedSlave, Checker&);

class Clock : public Model
{
private:
    cci::cci_param<int> ns_wait;
    cci::cci_param<int> ms_sleep;
    cci::cci_param<bool> record;
    bool running;

public:
    void entry() {
        running = true;
        while (running) {
            wait(sc_core::sc_time(ns_wait, SC_NS), checker.stopEvent);
            if (record)
                checker.record(this, Checker::ClockTick);

            if (ms_sleep && running) {
                checker.sleep_for(std::chrono::milliseconds(ms_sleep));
            }
        }
        checker.record(this, Checker::Finished);
    }
    Clock(sc_module_name _name, Checker& _checker)
        : Model(_name, _checker)
        , ns_wait("ns_wait", 10)
        , ms_sleep("ms_sleep", 0)
        , record("record", false)
        , running(false) {
        SCP_DEBUG(SCMOD) << "In the constructor Clock";
        SC_HAS_PROCESS(Clock);
        SC_METHOD(stop);
        dont_initialize();
        sensitive << checker.stopEvent;
    }
    void stop() { running = false; }

    void exit() {}
};
GSC_MODULE_REGISTER_1(Clock, Checker&);

class RealTimeClockLimiter : public Model
{
    gs::realtimeLimiter rtl;

public:
    void entry() {
        if (use)
            rtl.enable();
    }
    void exit() {}
    void stop() { rtl.disable(); }

    cci::cci_param<bool> use;
    RealTimeClockLimiter(sc_module_name _name, Checker& _checker)
        : Model(_name, _checker), use("use", true) {
        SCP_DEBUG(SCMOD) << "In the constructor RealTimeClockLimiter";
    }
};
GSC_MODULE_REGISTER_1(RealTimeClockLimiter, Checker&);

SC_MODULE (tests) {
    Checker& checker;

    SC_MODULE (atest) {
        Checker& checker;

        std::vector<Model*> allModels;
        std::vector<gs::InLineSync*> inlinesyncs;
        std::vector<sc_process_handle> handles;
        gs::Router<> r;

    public:
        cci::cci_param<int> ns_run;
        cci::cci_param<int> ns_quantum;
        cci::cci_param<int> rerun;
        cci::cci_param<int> seed;
        cci::cci_param<std::string> description;

        atest(sc_module_name _name, Checker & _checker)
            : sc_module(_name)
            , checker(_checker)
            , r("router")
            , ns_run("ns_run", 1000)
            , ns_quantum("ns_quantum", 10000)
            , rerun("rerun", 1)
            , seed("seed", 1)
            , description("description", "") {
            SCP_DEBUG(SCMOD) << "In the constructor atest";
            for (auto name : gs::sc_cci_children(name())) {
                cci::cci_broker_handle m_broker = cci::cci_get_broker();
                if (m_broker.has_preset_value(std::string(sc_module::name()) + "." + name +
                                              ".moduletype")) {
                    std::string type = m_broker
                                           .get_preset_cci_value(std::string(sc_module::name()) +
                                                                 "." + name + ".moduletype")
                                           .get_string();
                    Model* m = (Model*)gs::ModuleFactory::Create(type, name.c_str(), checker);
                    if (m) {
                        //                        std::cout << "adding a "<<type<<" with name
                        //                        "<<std::string(sc_module::name())+"."+name<<"\n";
                        allModels.push_back(m);
                    } else {
                        SCP_INFO(SCMOD) << "Can't find " << type;
                    }
                } // else it's some other config
            }

            /* wire all initiators to router */
            for (auto m : allModels) {
                if (Initiator* i = dynamic_cast<Initiator*>(m)) {
                    r.add_initiator(i->initiator_socket);
                }
            }

            for (auto m : allModels) {
                if (Target* s = dynamic_cast<Target*>(m)) {
                    if (s->requires_systemc()) {
                        gs::InLineSync* i = new gs::InLineSync(
                            (gs::sc_cci_leaf_name(s->name()) + "_ils").c_str());
                        inlinesyncs.push_back(i);
                        i->initiator_socket(s->target_socket);
                        r.add_target(i->target_socket, s->add_base, s->add_size);
                    } else {
                        r.add_target(s->target_socket, s->add_base, s->add_size);
                    }
                }
            }
        }

        void entry() {
            checker.record(this, Checker::StartTest);
            if (seed == 0)
                std::srand(std::time(nullptr));
            else
                std::srand(seed);

            tlm_utils::tlm_quantumkeeper::set_global_quantum(sc_time(ns_quantum, SC_NS));
            checker.record(this, Checker::SetQuantum, sc_time(ns_quantum, SC_NS));

            for (auto m : allModels) {
                handles.push_back(sc_spawn(sc_bind(&Model::entry, m)));
            }
        }
        void stop() {
            checker.record(this, Checker::StopTest);
            for (auto m : allModels) {
                m->stop();
            }
        }

        void exit() {
            for (auto m : allModels) {
                m->exit();
            }
            for (auto h : handles) {
                if (!h.terminated())
                    h.disable(SC_INCLUDE_DESCENDANTS);
            }
        }
    };

    std::vector<atest*> allTests;

public:
    tests(sc_module_name _name, Checker & _checker): sc_module(_name), checker(_checker) {
        SCP_DEBUG(SCMOD) << "In the constructor tests";
        for (auto name : gs::sc_cci_children(name())) {
            allTests.push_back(new atest(name.c_str(), checker));
        }
        //        std::sort (allTests.begin(), allTests.end(), [](const atest *a, const atest
        //        *b)->bool{return strcmp(a->name() , b->name());});
    }

    void runAll() {
        sc_start(10, SC_MS); // Dont want to start at time zero !
        for (auto t : allTests) {
            for (int i = 0; i < t->rerun; i++) {
                checker.startRecording(t->name());
                t->entry();
                sc_start(t->ns_run, SC_NS);
                t->stop(); // make sure everybody has recorded that they should stop
                checker.stopRecording();
                checker.stopEvent.notify(SC_ZERO_TIME); // notify, to ensure
                                                        // people come out of
                                                        // their waits

                sc_start(50, SC_MS);
                // drain everything
                while (sc_pending_activity()) { // process/propogate pending events
                    sc_core::sc_time t = sc_time_to_pending_activity();
                    sc_start(t);
                }

                t->exit();
                sc_start(50, SC_MS);
            }
            checker.finishedTest();
        }
        checker.allDone();
    }
};

int sc_main(int argc, char** argv) {
    scp::init_logging(scp::LogConfig()
                          .logAsync(false)
                          .printSimTime(false)
                          .logLevel(scp::log::DBGTRACE) // set log level to DBGTRACE = TRACEALL
                          .msgTypeFieldWidth(50));      // make the msg type column a bit tighter

    auto m_broker = new cci_utils::broker("Global Broker");
    cci::cci_register_broker(m_broker);
    LuaFile_Tool lua("lua");
    lua.parseCommandLine(argc, argv);

    auto uncon = m_broker->get_unconsumed_preset_values();
    for (auto v : uncon) {
        SCP_INFO("sc_main") << "Unconsumed config value: " << v.first << " : " << v.second;
    }

    try {
        Checker checker;
        tests allTests("tests", checker);

        allTests.runAll();

    } catch (sc_report rpt) {
        SCP_INFO("sc_main") << rpt.get_process_name();
        SCP_INFO("sc_main") << rpt.what();
    }

    printf("End!\n");

    return 0;
}

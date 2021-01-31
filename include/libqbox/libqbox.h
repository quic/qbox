/*
 *  This file is part of libqbox
 *  Copyright (c) 2019 Clement Deschamps and Luc Michel
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include <sstream>
#include <mutex>

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#ifndef _WIN32
#include <cci_configuration>
#endif

#include <libqemu-cxx/libqemu-cxx.h>

#include <libgs/libgs.h>

#include "loader.h"

#if 0
#define MLOG(foo, bar) \
    std::cout << "[" << name() << "] "
#else
#include <fstream>
#define MLOG(foo, bar) \
    if (0) std::cout
#endif

struct lock_extension : tlm::tlm_extension<lock_extension> {
    int lock;
    int source;

    lock_extension()
    {
        lock = 0;
        source = -1;
    }

    virtual tlm_extension_base* clone() const
    {
        lock_extension* ext = new lock_extension;
        ext->lock = lock;
        ext->source = source;
        return ext;
    }

    virtual void copy_from(tlm_extension_base const& ext)
    {
        lock = static_cast<lock_extension const &>(ext).lock;
        source = static_cast<lock_extension const &>(ext).source;
    }
};

class QemuComponent : public sc_core::sc_module
{
private:
    const std::string m_qemu_obj_id;

protected:
    qemu::LibQemu* m_lib = nullptr;
    qemu::Object m_obj;

public:
    cci::cci_param<bool> coroutines;

    SC_HAS_PROCESS(QemuComponent);

    QemuComponent(sc_core::sc_module_name name, const char* qemu_obj_id)
        : sc_module(name)
        , m_qemu_obj_id(qemu_obj_id)
        , coroutines("coroutines", false, "Run using coroutines in Qemu")
    {
    }

    virtual ~QemuComponent() {}

    void before_end_of_elaboration()
    {
    }

    std::vector<std::string> m_extra_qemu_args;

    void add_extra_qemu_args(std::initializer_list<const char *> args)
    {
        for (auto arg: args) {
            char * dst = new char[std::strlen(arg)+1];
            std::strcpy(dst, arg);
            m_extra_qemu_args.push_back(dst);
        }
    }

    void qemu_init(const std::string &arch, int icount, bool singlestep, int gdb_port, std::string trace, std::string semihosting, gs::SyncPolicy::Type type, std::vector<std::string> &extra_qemu_args)
    {
        m_lib = new qemu::LibQemu(*new LibQemuLibraryLoader());

        m_lib->push_qemu_arg("LIBQEMU");
        m_lib->push_qemu_arg({ "-M", "none" });
        m_lib->push_qemu_arg({ "-m", "2048" }); /* Used by QEMU to set the TB cache size */
        m_lib->push_qemu_arg({ "-monitor", "null" });
        m_lib->push_qemu_arg({ "-serial", "null" });
        m_lib->push_qemu_arg({ "-nographic" });

        switch (type) {
            case gs::SyncPolicy::SYSTEMC_THREAD:
                if (coroutines) {
                    m_lib->push_qemu_arg({ "-accel", "tcg,coroutine=on,thread=single" });
                } else {
                    m_lib->push_qemu_arg({ "-accel", "tcg,thread=single" });
                }
                break;
            case gs::SyncPolicy::OS_THREAD:
                if (coroutines) {
                    m_lib->push_qemu_arg({ "-accel", "tcg,coroutine=on,thread=single" });
                } else {
                    m_lib->push_qemu_arg({ "-accel", "tcg,thread=single" });
                }
                break;
        }

        if (icount >= 0) {
            std::stringstream ss;
            ss << icount << ",nosleep";
            m_lib->push_qemu_arg({ "-icount", ss.str().c_str() });
        }

        if (singlestep) {
            m_lib->push_qemu_arg({ "-singlestep" });
        }

        if (gdb_port >= 0) {
            m_lib->push_qemu_arg({ "-S" });
        }

        {
            std::stringstream ss;
            ss << "qemu-" << name() << ".log";
            m_lib->push_qemu_arg({ "-D", ss.str().c_str() });
        }

        if (!trace.empty()) {
            m_lib->push_qemu_arg({ "-d", trace.c_str() });
        }

        if (!semihosting.empty()) {
            std::stringstream ss;
            ss << "enable=on,target=" << semihosting;
            m_lib->push_qemu_arg({ "-semihosting-config", ss.str().c_str() });
        }

        for (std::string &arg : extra_qemu_args) {
            m_lib->push_qemu_arg(arg.c_str());
        }

        {
            std::stringstream ss;
#ifdef _WIN32
            ss << "libqemu-system-" << arch << ".dll";
#else
            ss << "libqemu-system-" << arch << ".so";
#endif
            m_lib->init(ss.str().c_str());
        }
    }

    bool realized = false;

    void realize() {
        if (!realized) {
            get_qemu_obj().set_prop_bool("realized", true);
            realized = true;
        }
    }

    void end_of_elaboration() {
        realize();
    }

    void start_of_simulation()
    {
        m_lib->resume_all_vcpus();
    }

    const std::string& get_qemu_obj_id() const { return m_qemu_obj_id; }
    qemu::Object get_qemu_obj() { return m_obj; }
    qemu::LibQemu& get_qemu_inst() { return *m_lib; }

    void set_qemu_instance(qemu::LibQemu& lib) {
        m_lib = &lib;
        m_obj = m_lib->object_new(m_qemu_obj_id.c_str());

        set_qemu_instance_callback();
    }

    virtual void set_qemu_instance_callback() {}

    virtual void set_qemu_properties_callback() { }
};

template <unsigned int BUSWIDTH = 32>
class TlmInitiatorPort {
public:
    tlm_utils::simple_initiator_socket<TlmInitiatorPort>* socket;

    std::string m_name;

    TlmInitiatorPort(const char* name)
    {
        m_name = name;

        // TODO: sc_find_object
    }

    std::string name()
    {
        return m_name;
    }
};

class QemuInPort : public sc_core::sc_in<bool> {
    QemuComponent& m_comp;
    int m_gpio_idx;

public:
    QemuInPort(const std::string& name, QemuComponent& comp, int gpio_idx)
        : sc_in(name.c_str())
        , m_comp(comp)
    {
        m_gpio_idx = gpio_idx;
    }

    virtual void before_end_of_elaboration()
    {
    }

    qemu::Gpio get_gpio()
    {
        qemu::Device dev(m_comp.get_qemu_obj());

        const char* name = "";
        int idx = m_gpio_idx;

        if (*name) {
            return dev.get_gpio_in_named(name, idx);
        }
        else {
            return dev.get_gpio_in(idx);
        }
    }

    void watch_port()
    {
        const sc_core::sc_event& ev = default_event();

        for (;;) {
            sc_core::wait(ev);

            bool value = read();

            //printf("QemuInPort: [%d]=%d\n", m_gpio_idx, value);

            qemu::Gpio gpio(get_gpio());
            gpio.set(value);
        }
    }

    virtual void end_of_elaboration()
    {
#if 0
        InPort::end_of_elaboration();

        if (m_cs.is_selected()) {
            m_default_ev = &m_qemu_gpio_event;
        }
        else
#else
        {

            /* SystemC to QEMU connection. We must setup the SystemC thread
             * that will watch the gpio and update QEMU accordingly */
            sc_core::sc_spawn(std::bind(&QemuInPort::watch_port, this));

            //m_default_ev = &default_event();
#endif
        }
    }
};

#ifdef _WIN32
template <class T>
class gs_param : public sc_core::sc_object {
public:
    sc_core::sc_attribute<T> m_value;

    gs_param(const char *name, T value, const char *desc)
        : sc_object(name)
        , m_value("value", value)
    {
        add_attribute(m_value);
    }

    gs_param& operator= (const T& value)
    {
        m_value.value = value;
        return *this;
    }

    operator const T& () const
    {
        return m_value.value;
    }
};
#endif

class QemuCpu : public QemuComponent {
    gs::RunOnSysC onSystemC; // Private SystemC helper
    std::mutex mutex;
    std::shared_ptr<qemu::Timer> m_deadline_timer;
    sc_core::sc_time m_run_budget;
public:
    const std::string m_arch;
    qemu::Cpu m_cpu;

    tlm_utils::simple_initiator_socket<QemuCpu> socket;

    sc_core::sc_port<sc_core::sc_signal_in_if<bool>, 1, sc_core::SC_ZERO_OR_MORE_BOUND> reset;

#ifndef _WIN32
    cci::cci_param<int> icount;
    cci::cci_param<bool> singlestep;
    cci::cci_param<int> gdb_port;
    cci::cci_param<std::string> trace;
    cci::cci_param<std::string> semihosting;
    cci::cci_param<std::string> sync_policy;
    cci::cci_param<int> quantum_ns;
    cci::cci_param<bool> threadsafe_access;
#else
    gs_param<int> icount;
    gs_param<bool> singlestep;
    gs_param<int> gdb_port;
    gs_param<std::string> trace;
    gs_param<std::string> semihosting;
    gs_param<std::string> sync_policy;
    gs_param<int> quantum_ns;
    gs_param<bool> threadsafe_access;
#endif

    unsigned m_max_access_size;

    /* A CPU address space */
    struct AddressSpace {
        qemu::MemoryRegion mr;      /* The associated MR */
        TlmInitiatorPort<>* port;   /* The TLM port */
        const char* prop;          /* The name of the CPU property mapping to this AS */
    };

    std::vector<AddressSpace> m_ases;

    sc_core::sc_event_or_list m_external_ev;
    std::shared_ptr<sc_core::sc_event> m_cross_cpu_exec_ev;

    int64_t m_last_vclock;

    std::shared_ptr<gs::tlm_quantumkeeper_extended> m_qk;

    void wait_for_work()
    {
        if (gdb_port == 0) {
            wait(m_external_ev);
        } else {
            if (sc_core::sc_pending_activity()) {
                wait(sc_core::sc_time_to_pending_activity());
            } else {
                /* Spin */
                wait(sc_core::SC_ZERO_TIME);
            }
        }
    }

    void mainloop_thread_coroutine()
    {
        m_qk->start();
        std::shared_ptr<qemu::Timer> deadline_timer;
        {
            std::lock_guard<std::mutex> lock(mutex);
            deadline_timer = m_lib->timer_new();
        }

        sc_core::sc_time run_budget;

        deadline_timer->set_callback([this]() { m_cpu.kick();});
        if (!m_cpu.can_run()) {
            MLOG(SIM, DBG) << "No more cpu work\n";
            wait_for_work();
        }

        while (m_cpu.loop_is_busy()) {
            MLOG(SIM, DBG) << "Another CPU is running, waiting...\n";
            wait(*m_cross_cpu_exec_ev);
        }

        m_cpu.register_thread();

        for (;;) {
            sc_core::sc_time elapsed;

            if (!m_cpu.can_run()) {
                m_qk->sync();
            }

            run_budget = m_qk->time_to_sync();
            if (run_budget == sc_core::sc_time(sc_core::SC_ZERO_TIME)) {
                wait(run_budget);
                continue;
            }
            MLOG(SIM, TRC) << "CPU run, budget: " << run_budget << "\n";
            int64_t budget = int64_t(run_budget.to_seconds() * 1e9);

            m_last_vclock = m_lib->get_virtual_clock();
            int64_t next_point = m_last_vclock + budget;
            deadline_timer->mod(next_point);

            m_cpu.loop();
            if (m_lib->get_virtual_clock() == m_last_vclock) {
                m_cpu.loop();
            }

            m_cross_cpu_exec_ev->notify();

            int64_t now = m_lib->get_virtual_clock();

            deadline_timer->del();

            elapsed = sc_core::sc_time(now - m_last_vclock, sc_core::SC_NS);
            m_qk->inc(elapsed);

            MLOG(SIM, TRC) << "elapsed: " << elapsed << "\n";

            if (m_qk->need_sync()) {
                m_qk->sync();
            }
        }
    }

    struct dmi_region {
        uint64_t start;
        uint64_t end;
        uint8_t *ptr;
        qemu::MemoryRegion mr;

        dmi_region(uint64_t start, uint64_t end, uint8_t *ptr, qemu::MemoryRegion mr)
        {
            this->start = start;
            this->end = end;
            this->ptr = ptr;
            this->mr = mr;
        }
    };

    void dump_dmis() {
#if 0
        printf("%s.dmi_aliases: (%lu)\n", name(), dmi_aliases.size());
        for (auto &r : dmi_aliases) {
            printf("  [0x%08lx:0x%08lx] ptr=%p\n", r.start, r.end, r.ptr);
        }
#endif
    }

    struct dmi_inval_args {
        QemuCpu *cpu;
        uint64_t start;
        uint64_t end;
        std::vector<qemu::MemoryRegion> mrs;
    };

    static bool dmi_force_exit_on_io;

    static void do_dmi_inval(void *opaque)
    {
        struct dmi_inval_args *args = (struct dmi_inval_args *)opaque;

        args->cpu->m_lib->tb_invalidate_phys_range(args->start, args->end);

        args->cpu->m_lib->lock_iothread();

        for (auto &mr : args->mrs) {
            mr.container->del_subregion(mr);
        }

        args->cpu->m_lib->unlock_iothread();

        dmi_force_exit_on_io = 0;

        delete args;
    }

    virtual void invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end)
    {
        std::vector<qemu::MemoryRegion> mrs;

        for (auto it = dmi_aliases.begin(); it != dmi_aliases.end(); it++) {
            auto r = *it;
            if (start <= r.end && end >= r.start) {
                mrs.push_back(r.mr);
                dmi_aliases.erase(it--);
            }
        }

        if (mrs.size() > 0) {
            struct dmi_inval_args *args = new struct dmi_inval_args;
            args->cpu = this;
            args->start = start;
            args->end = end;
            args->mrs = mrs;
            m_cpu.async_safe_run(do_dmi_inval, args);

            dmi_force_exit_on_io = 1;

            dump_dmis();
        }
    }

    /* global dmi regions (must not intersect with each other) */
    static std::vector<struct dmi_region> g_dmis;

    /* local aliases to dmi regions */
    std::vector<struct dmi_region> dmi_aliases;

    void add_dmi_region(uint64_t start, uint64_t end, uint8_t *ptr)
    {
        uint64_t size = end - start + 1;

        qemu::MemoryRegion mr = m_lib->object_new<qemu::MemoryRegion>();
        mr.init_ram_ptr(m_obj, "dmi", size, ptr);

        struct dmi_region r(start, end, ptr, mr);
        g_dmis.push_back(r); /* TODO: sort */
    }

    void check_dmi_hint(tlm::tlm_generic_payload &trans, AddressSpace &as)
    {
        tlm::tlm_dmi dmi_data;
        uint64_t addr = trans.get_address();
        uint64_t len = trans.get_data_length();
        uint64_t dmi_start;
        uint64_t dmi_end;
        uint8_t *dmi_ptr;

        if (!trans.is_dmi_allowed()) {
            return;
        }

        /* check if there is already a dmi region covering this range */
        for (auto &r : dmi_aliases) {
            if (addr >= r.start && (addr + len - 1) <= r.end) {
                return;
            }
        }

        bool dmi_ptr_valid = socket->get_direct_mem_ptr(trans, dmi_data);
        if (!dmi_ptr_valid) {
            return;
        }

        /* phase 1: create qemu memory regions */

        dmi_start = dmi_data.get_start_address();
        dmi_end = dmi_data.get_end_address();
        dmi_ptr = dmi_data.get_dmi_ptr();

        for (auto &r : g_dmis) {
            if (dmi_start <= r.end && dmi_end >= r.start) {
                /* intersection */
                if (dmi_start < r.start) {
                    /* new dmi region starts before existing dmi region */
                    add_dmi_region(dmi_start, r.start - 1, dmi_ptr);
                }
                dmi_ptr += r.end - dmi_start + 1;
                dmi_start = r.end + 1;
            }
        }
        if (dmi_end > dmi_start) {
            add_dmi_region(dmi_start, dmi_end, dmi_ptr);
        }
        /* TODO: merge adjacent regions (require invalidating aliases) */

        /* phase 2: alias qemu memory regions */

        for (auto &r : g_dmis) {
            if (addr >= r.start && (addr + len - 1) <= r.end) {
                uint64_t size = r.end - r.start + 1;
                qemu::MemoryRegion mr = m_lib->object_new<qemu::MemoryRegion>();
                mr.init_alias(m_obj, "dmi-alias", r.mr, 0, size);
                as.mr.add_subregion(mr, r.start);

                struct dmi_region r(r.start, r.end, r.ptr, mr);
                dmi_aliases.push_back(r); /* TODO: sort */
            }
        }

        dump_dmis();
    }

    /**
     *  Called before b_transport. Can be overloaded by cpu specialization.
     *  Return false for skipping b_transport
     */
    virtual bool before_b_transport(tlm::tlm_generic_payload &trans,
            qemu::MemoryRegionOps::MemTxAttrs &attrs)
    {
        return true;
    }

    /**
     *  Called after b_transport. Can be overloaded by cpu specialization.
     */
    virtual void after_b_transport(tlm::tlm_generic_payload &trans,
            qemu::MemoryRegionOps::MemTxAttrs &attrs)
    {
    }

    qemu::MemoryRegionOps::MemTxResult
    qemu_io_access(AddressSpace& as, tlm::tlm_command command,
                   uint64_t addr, uint64_t* val, unsigned int size,
                   qemu::MemoryRegionOps::MemTxAttrs attrs)
    {
        int64_t before = m_lib->get_virtual_clock();
        sc_core::sc_time local_drift, local_drift_before;
        tlm::tlm_generic_payload trans;

        m_lib->unlock_iothread();

        trans.set_command(command);
        trans.set_address(addr);
        trans.set_data_ptr(reinterpret_cast<unsigned char*>(val));
        trans.set_data_length(size);
        trans.set_streaming_width(size);
        trans.set_byte_enable_length(0);
        trans.set_dmi_allowed(false);
        trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        if (attrs.debug) {
            qemu::MemoryRegionOps::MemTxResult ret;
            std::function<void()> job=[this, &trans] {
                socket->transport_dbg(trans);
            };
            onSystemC.run_on_sysc(job);

            switch (trans.get_response_status()) {
                case tlm::TLM_OK_RESPONSE:
                    ret = qemu::MemoryRegionOps::MemTxOK;
                    break;
                case tlm::TLM_ADDRESS_ERROR_RESPONSE:
                    ret = qemu::MemoryRegionOps::MemTxDecodeError;
                    break;
                default:
                    ret = qemu::MemoryRegionOps::MemTxError;
                    break;
            }

            m_lib->lock_iothread();
            return ret;
        }

        sc_core::sc_time elapsed(before - m_last_vclock, sc_core::SC_NS);

        m_qk->inc(elapsed);

        std::function<void()> job = [this, &trans, &attrs] {
            sc_core::sc_time delay = m_qk->get_local_time();
            if (!before_b_transport(trans, attrs)) {
                return;
            }
            socket->b_transport(trans, delay);
            m_qk->set(delay);
        };

        if (threadsafe_access) {
            std::lock_guard<std::mutex> lock(mutex);
            job();
        } else {
            onSystemC.run_on_sysc(job);
        }

        /* reset transaction address before dmi check (could be altered by b_transport) */
        trans.set_address(addr);

        after_b_transport(trans, attrs);
        check_dmi_hint(trans, as);

        /*
         * NB it is unsafe to 'sync' from within an io access as potentially
         * Qemu will need to do exclusive work, and will deadlock
         */
        if (m_qk->need_sync())
            m_cpu.request_exit();

        m_lib->lock_iothread();

        m_last_vclock = m_lib->get_virtual_clock();

        auto sta = trans.get_response_status();

        if (dmi_force_exit_on_io) {
            if (sta == tlm::TLM_OK_RESPONSE) {
                return qemu::MemoryRegionOps::MemTxOKExitTB;
            }
            /* in case of error, QEMU will exit the TB anyway */
        }

        if (sta == tlm::TLM_OK_RESPONSE) {
            return qemu::MemoryRegionOps::MemTxOK;
        }
        else if (sta == tlm::TLM_ADDRESS_ERROR_RESPONSE) {
            return qemu::MemoryRegionOps::MemTxDecodeError;
        }
        return qemu::MemoryRegionOps::MemTxError;
    }

    qemu::MemoryRegionOps::MemTxResult
    qemu_io_read(AddressSpace& as,
                 uint64_t addr, uint64_t* val, unsigned int size,
                 qemu::MemoryRegionOps::MemTxAttrs attrs)
    {
        return qemu_io_access(as, tlm::TLM_READ_COMMAND, addr, val, size, attrs);
    }

    qemu::MemoryRegionOps::MemTxResult
    qemu_io_write(AddressSpace& as,
                  uint64_t addr, uint64_t val, unsigned int size,
                  qemu::MemoryRegionOps::MemTxAttrs attrs)
    {
        return qemu_io_access(as, tlm::TLM_WRITE_COMMAND, addr, &val, size, attrs);
    }

    void add_initiator(const char* port_name, const char* mr_link_name)
    {
        TlmInitiatorPort<>* initiator = new TlmInitiatorPort<>(port_name);
        m_ases.emplace_back(AddressSpace{ qemu::MemoryRegion(), initiator, mr_link_name });
    }

    void connect_initator(AddressSpace& as)
    {
        as.mr = m_lib->object_new<qemu::MemoryRegion>();
        qemu::MemoryRegionOpsPtr ops = m_lib->memory_region_ops_new();

        ops->set_read_callback(
            [this, &as](uint64_t addr, uint64_t* val, unsigned int size,
                qemu::MemoryRegionOps::MemTxAttrs attrs) {
                    return qemu_io_read(as, addr, val, size, attrs);
            });

        ops->set_write_callback(
            [this, &as](uint64_t addr, uint64_t val, unsigned int size,
                qemu::MemoryRegionOps::MemTxAttrs attrs) {
                    return qemu_io_write(as, addr, val, size, attrs);
            });

        ops->set_max_access_size(m_max_access_size);

        /* The root memory region maps the whole address space */
        as.mr.init_io(m_obj, as.port->name().c_str(), (std::numeric_limits<uint64_t>::max)(), ops);

        m_obj.set_prop_link(as.prop, as.mr);

        MLOG(APP, TRC) << "created initiator `" << as.port->name()
            << "` connected to address space `" << as.prop << "`\n";
    }

    void connect_initators()
    {
        for (AddressSpace& as : m_ases) {
            connect_initator(as);
        }
    }

public:
    SC_HAS_PROCESS(QemuCpu);

    QemuCpu(sc_core::sc_module_name name, const std::string arch_name, const std::string type_name)
        : QemuComponent(name, (type_name + "-cpu").c_str())
        , m_arch(arch_name)
        , reset("reset")
        , icount("icount", 1, "Enable virtual instruction counter")
        , singlestep("singlestep", false, "Run the emulation in single step mode")
        , gdb_port("gdb_port", -1, "Wait for gdb connection on TCP port <gdb_port>")
        , trace("trace", "", "Specify tracing options")
        , semihosting("semihosting", "", "Enable and configure semihosting ")
        , sync_policy("sync_policy", "multithread-quantum", "Synchronization Policy to use")
        , quantum_ns("quantum", 10000, "TLM-2.0 Quantum in ns")
        , threadsafe_access("threadsafe_access", false, "Qemu calls b_transport from tcg thread")
        , m_last_vclock(0)
    {
        m_max_access_size = 4;
        tlm_utils::tlm_quantumkeeper::set_global_quantum(sc_core::sc_time(quantum_ns,sc_core::SC_NS));
        socket.register_invalidate_direct_mem_ptr(this, &QemuCpu::invalidate_direct_mem_ptr);
    }

    virtual ~QemuCpu() {
        m_cpu.halt(true);
    }

    void before_end_of_elaboration()
    {
        m_qk = gs::tlm_quantumkeeper_factory(sync_policy);
        if (!m_qk) {
            SC_REPORT_FATAL("qbox", "Sync policy unknown");
        }
        m_qk->reset();

        for (QemuComponent *c : m_nearby_components) {
            m_extra_qemu_args.insert(m_extra_qemu_args.end(), c->m_extra_qemu_args.begin(), c->m_extra_qemu_args.end());
        }

        if (!coroutines && (icount < 0)) {
            SC_REPORT_FATAL("qbox", "Cannot use TCG without icount");
        }

        if (!m_lib) {
            printf("create new qemu instance for cpu %s\n", name());
            qemu_init(m_arch, icount, singlestep, gdb_port, trace, semihosting,  m_qk->get_thread_type(), m_extra_qemu_args);
        }
        else {
            printf("use existing qemu instance for cpu %s\n", name());
        }

        QemuComponent::before_end_of_elaboration();

        /* The default address space */
        add_initiator("mem", "memory");

        set_qemu_instance(*m_lib);

        if (coroutines)
            sc_core::sc_spawn(std::bind(&QemuCpu::mainloop_thread_coroutine, this));

        static std::shared_ptr<sc_core::sc_event> ev = std::make_shared<sc_core::sc_event>();
        set_cross_cpu_exec_event(ev);

        for (QemuComponent *c : m_nearby_components) {
            c->set_qemu_instance(*m_lib);
        }
    }

    virtual void reset_begin()
    {
        m_cpu.halt(true);
    }

    virtual void reset_end()
    {
        m_cpu.reset();
        m_cpu.halt(false);
    }

    virtual void end_of_elaboration()
    {
        using std::string;

        set_qemu_properties_callback();

        QemuComponent::end_of_elaboration();

        sc_core::sc_signal_in_if<bool> *reset_i_f = reset.get_interface(0);
        if (reset_i_f) {
            SC_METHOD(reset_begin);
            sc_core::sc_module::sensitive << reset_i_f->negedge_event();

            SC_METHOD(reset_end);
            sc_core::sc_module::sensitive << reset_i_f->posedge_event();

            m_external_ev |= reset_i_f->negedge_event();
            m_external_ev |= reset_i_f->posedge_event();
        }

#if 0
        bool gdb_server_alias = m_params["create-gdb-server-alias"].template as<bool>();
        if (gdb_server_alias) {
            Component::get_context().add_param_alias("gdb-server", m_params["gdb-server"]);
        }
#endif

        if (gdb_port >= 0) {
            std::stringstream ss;
            ss << gdb_port;
            MLOG(APP, INF) << "Starting gdb server on port " << ss.str() << "\n";
            m_lib->start_gdb_server("tcp::" + ss.str());
        }

        if (!coroutines) {
            m_qk->start();
            {
                std::lock_guard<std::mutex> lock(mutex);
                m_deadline_timer = m_lib->timer_new();
            }

            m_deadline_timer->set_callback([this]() {
                // Unlock iothread when interacting with systemc
                m_lib->unlock_iothread();

                m_deadline_timer->del();
                m_qk->inc(m_run_budget);
                if (m_run_budget != sc_core::SC_ZERO_TIME) {
                    GS_LOG("QEMU ran, now syncing");
                    m_qk->sync();
                }
                if (sc_core::sc_get_status() == sc_core::sc_status::SC_STOPPED) {
                    m_lib->lock_iothread();
                    return;
                }
                m_run_budget = m_qk->time_to_sync();
                while (m_run_budget == sc_core::SC_ZERO_TIME) {
                    GS_LOG("QEMU run budget is 0, waiting");
                    onSystemC.run_on_sysc([](){sc_core::wait(sc_core::SC_ZERO_TIME);}, true);
                    if (sc_core::sc_get_status() == sc_core::sc_status::SC_STOPPED) {
                        m_lib->lock_iothread();
                        return;
                    }
                    m_run_budget = m_qk->time_to_sync();
                }
                GS_LOG("QEMU run budget is %s, running", m_run_budget.to_string().c_str());

                m_lib->lock_iothread();

                int64_t budget = int64_t(m_run_budget.to_seconds() * 1e9);
                m_last_vclock = m_lib->get_virtual_clock();
                int64_t next_point = m_last_vclock + budget;
                m_deadline_timer->mod(next_point);
            });

            // run one quantum initially
            m_run_budget = m_qk->time_to_sync();
            int64_t budget = int64_t(m_run_budget.to_seconds() * 1e9);
            m_last_vclock = m_lib->get_virtual_clock();
            int64_t next_point = m_last_vclock + budget;
            m_deadline_timer->mod(next_point);
        }
    }

    virtual void set_qemu_instance_callback()
    {
        QemuComponent::set_qemu_instance_callback();
        m_cpu = qemu::Cpu(m_obj);
    }

    virtual void set_qemu_properties_callback()
    {
        QemuComponent::set_qemu_properties_callback();
        connect_initators();
    }

    void set_cross_cpu_exec_event(std::shared_ptr<sc_core::sc_event> ev)
    {
        m_cross_cpu_exec_ev = ev;
    }

    std::vector<QemuComponent *> m_nearby_components;

    void add_to_qemu_instance(QemuComponent *c)
    {
        m_nearby_components.push_back(c);
    }
};

class QemuSignal : public sc_core::sc_signal<bool> {
public:
    sc_core::sc_port_base* bound_port;

    QemuSignal(sc_core::sc_module_name nm) : sc_signal<bool>(nm), bound_port(NULL)
    {
    }

    void register_port(sc_core::sc_port_base& port, const char *)
    {
        bound_port = &port;
    }
};

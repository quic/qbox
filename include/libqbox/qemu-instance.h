/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Luc Michel
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

#ifndef LIBQBOX_QEMU_INSTANCE_H_
#define LIBQBOX_QEMU_INSTANCE_H_

#include <cassert>
#include <sstream>
#include <systemc>

#include <cci_configuration>
#include <vector>

#include <greensocs/gsutils/cciutils.h>
#include <greensocs/gsutils/report.h>
#include <greensocs/libgssync.h>

#include <libqemu-cxx/libqemu-cxx.h>

#include "libqbox/dmi-manager.h"
#include "libqbox/exceptions.h"


class QemuDeviceBaseIF {
public:
    virtual bool can_run() {
        return true;
    }
};
/**
 * @class QemuInstance
 *
 * @brief This class encapsulates a libqemu-cxx qemu::LibQemu instance. It
 * handles QEMU parameters and instance initialization.
 */
class QemuInstance : public sc_core::sc_module {
private:
    std::shared_ptr<gs::tlm_quantumkeeper_extended> m_first_qk=NULL;
    std::mutex m_lock;
    std::list<QemuDeviceBaseIF *> devices;
public:
    void add_dev(QemuDeviceBaseIF *d) {
        std::lock_guard<std::mutex> lock(m_lock);
        devices.push_back(d);
    }
    void del_dev(QemuDeviceBaseIF *d) {
        std::lock_guard<std::mutex> lock(m_lock);
        devices.remove(d);
    }
    bool can_run()
    {
        std::lock_guard<std::mutex> lock(m_lock);
        bool can_run=false;
        // In SINGLE mode, check if another CPU could run
        for (auto const& i : devices) {
            if (i->can_run()) {
                can_run=true;
                break;
            }
        }
        return can_run;
    }
    using Target = qemu::Target;
    using LibLoader = qemu::LibraryLoaderIface;

    enum TcgMode {
        TCG_UNSPECIFIED,
        TCG_SINGLE,
        TCG_COROUTINE,
        TCG_MULTI,
    };
    TcgMode StringToTcgMode(std::string s)
    {
        if (s == "SINGLE") return TCG_SINGLE;
        if (s == "COROUTINE") return TCG_COROUTINE;
        if (s == "MULTI") return TCG_MULTI;
        SC_REPORT_WARNING("libqbox",("Unknown TCG mode "+s).c_str());
        return TCG_UNSPECIFIED;
    }

protected:
    qemu::LibQemu m_inst;
    QemuInstanceDmiManager m_dmi_mgr;

    cci::cci_param<std::string> p_tcg_mode;
    cci::cci_param<std::string> p_sync_policy;
    TcgMode m_tcg_mode;

    cci::cci_param<bool> p_icount;
    cci::cci_param<int> p_icount_mips;

    cci::cci_param<std::string> p_args;

    void push_default_args()
    {
        auto m_conf_broker = cci::cci_get_broker();
        const size_t l = strlen(name()) + 1;

        m_inst.push_qemu_arg("libqbox"); /* argv[0] */
        m_inst.push_qemu_arg({
            "-M", "none", /* no machine */
            "-m", "2048", /* used by QEMU to set some interal buffer sizes */
            "-monitor", "null", /* no monitor */
            "-serial", "null", /* no serial backend */
            "-display", "none", /* no GUI */
        });

        const char* args = "args.";
        for (auto p : m_conf_broker.get_unconsumed_preset_values([&](const std::pair<std::string, cci::cci_value>& iv) { return iv.first.find(std::string(name()) + "." + args) == 0; })) {
            if (p.second.get_string().is_string()) {
                const std::string arg_name = p.first.substr(l + strlen(args));
                const std::string arg_value = p.second.get_string();
                SC_REPORT_INFO(name(), ("Added QEMU argument : " + arg_name + " " + arg_value).c_str());
                m_inst.push_qemu_arg({ arg_name.c_str(), arg_value.c_str() });
            } else {
                SC_REPORT_ERROR(name(), "The value of the argument is not a string");
            }
        }

        std::stringstream ss_args(p_args);
        std::string arg;
        while (ss_args >> arg) {
            m_inst.push_qemu_arg(arg.c_str());
        }
    }

    void push_icount_mode_args()
    {
        std::ostringstream ss;
        if (p_icount_mips > 0) {
            p_icount = true;
        }
        p_icount.lock();
        p_icount_mips.lock();
        if (!p_icount) return;
        if (m_tcg_mode == TCG_MULTI) {
            SC_REPORT_ERROR("libqbox", "MULTI threading can not be used with icount");
            assert(m_tcg_mode != TCG_MULTI);
        }
        m_inst.push_qemu_arg("-icount");

        ss << p_icount_mips << ",nosleep";
        m_inst.push_qemu_arg(ss.str().c_str());
    }

    void push_tcg_mode_args()
    {
        m_inst.push_qemu_arg("-accel");

        switch (m_tcg_mode) {
        case TCG_SINGLE:
            m_inst.push_qemu_arg("tcg,thread=single");
            break;

        case TCG_COROUTINE:
            m_inst.push_qemu_arg("tcg,thread=single,coroutine=on");
            break;

        case TCG_MULTI:
            m_inst.push_qemu_arg("tcg,thread=multi");
            if (p_icount) {
                SC_REPORT_ERROR("libqbox", "MULTI threading can not be used with icount");
                assert(!p_icount);
            }
            break;

        default:
            assert(false);
        }
    }

public:
    QemuInstance(const sc_core::sc_module_name& n, LibLoader& loader, Target t)
        : sc_core::sc_module(n)
        , m_inst(loader, t)
        , m_dmi_mgr(m_inst)
        , p_args("args", "", "additional space separated arguments")
        , p_tcg_mode("tcg_mode", "MULTI", "The TCG mode required, SINGLE, COROUTINE or MULTI")
        , p_sync_policy("sync_policy", "multithread-quantum", "Synchronization Policy to use")
        , m_tcg_mode(StringToTcgMode(p_tcg_mode))
        , p_icount("icount", false, "Enable virtual instruction counter")
        , p_icount_mips("icount_mips_shift", 0, "The MIPS shift value for icount mode (1 insn = 2^(mips) ns)")
    {
        p_tcg_mode.lock();
        push_default_args();
    }

    QemuInstance(const QemuInstance&) = delete;
    QemuInstance(QemuInstance&&) = delete;
    virtual ~QemuInstance() {}

    bool operator==(const QemuInstance& b) const
    {
        return this == &b;
    }

    bool operator!=(const QemuInstance& b) const
    {
        return this != &b;
    }

    /**
     * @brief Add a command line argument to the qemu instance.
     *
     * This method may only be called before the instance is initialized.
     */
    void add_arg(const char* arg)
    {
        m_inst.push_qemu_arg(arg);
    }

    /**
     * @brief Get the TCG mode for this instance
     *
     * @details This method is called by CPU instances determin if to use 
     * coroutines or not.
     */
    TcgMode get_tcg_mode()
    {
        return m_tcg_mode;
    }

    /**
     * @brief Get the TCG mode for this instance
     *
     * @details This method is called by CPU instances determin if to use
     * coroutines or not.
     */
    std::shared_ptr<gs::tlm_quantumkeeper_extended> create_quantum_keeper()
    {
        std::shared_ptr<gs::tlm_quantumkeeper_extended> qk;
        /* only multi-mode sync should have separate QK's per CPU */
        if (m_first_qk && m_tcg_mode!=TCG_MULTI) {
            qk=m_first_qk;
        } else {
            qk=gs::tlm_quantumkeeper_factory(p_sync_policy);
        }
        if (!m_first_qk) m_first_qk=qk;
        if (qk->get_thread_type()==gs::SyncPolicy::SYSTEMC_THREAD) {
            assert(m_tcg_mode==TCG_COROUTINE);
        }
        /* The p_sync_policy parameter should not be modified anymore */
        p_sync_policy.lock();
        return qk;
    }

    /**
     * @brief Initialize the QEMU instance
     *
     * @details Initialize the QEMU instance with the set TCG and icount mode.
     * If the TCG mode hasn't been set, it defaults to TCG_SINGLE.
     * If icount mode hasn't been set, it defaults to ICOUNT_OFF.
     *
     * The instance should not already be initialized when calling this method.
     */
    void init()
    {
        assert(!is_inited());

        if (m_tcg_mode == TCG_UNSPECIFIED) {
            SC_REPORT_ERROR("libqbox", ("Unknown tcg mode : " + std::string(p_tcg_mode)).c_str());
        }
        // By now, if there is a CPU, it would be loaded into QEMU, and we would have a QK
        if (m_first_qk) {
            if (m_first_qk->get_thread_type()==gs::SyncPolicy::SYSTEMC_THREAD) {
                if (p_tcg_mode.is_preset_value() && m_tcg_mode!=TCG_COROUTINE) {
                    SC_REPORT_WARNING("libqbox", "This quantum keeper can only be used with TCG_COROUTINES");
                }
                m_tcg_mode=TCG_COROUTINE;
            } else {
                if (m_tcg_mode == TCG_COROUTINE) {
                    SC_REPORT_ERROR("libqbox", "Please select a suitable threading mode for this quantum keeper, it can't be used with COROUTINES");
                }
            }

        }

        push_tcg_mode_args();
        push_icount_mode_args();

        GS_LOG("Initializing QEMU instance with args:");

        for (const char* arg : m_inst.get_qemu_args()) {
            GS_LOG("%s", arg);
        }

        m_inst.init();
    }

    /**
     * @brief Returns true if the instance is initialized
     */
    bool is_inited() const
    {
        return m_inst.is_inited();
    }

    /**
     * @brief Returns the underlying qemu::LibQemu instance
     *
     * @details Returns the underlying qemu::LibQemu instance. If the instance
     * hasn't been initialized, init is called just before returning the
     * instance.
     */
    qemu::LibQemu& get()
    {
        if (!is_inited()) {
            init();
        }

        return m_inst;
    }

    /**
     * @brief Returns the locked QemuInstanceDmiManager instance
     *
     * Note: we rely on RVO here so no copy happen on return (this is enforced
     * by the fact that the LockedQemuInstanceDmiManager copy constructor is
     * deleted).
     */
    LockedQemuInstanceDmiManager get_dmi_manager()
    {
        return LockedQemuInstanceDmiManager(m_dmi_mgr);
    }
};

/**
 * @class QemuInstanceManager
 *
 * @brief QEMU instance manager class
 *
 * @details This class manages QEMU instances. It allows to create instances
 * using the same library loader, thus allowing multiple instances of the same
 * library being loaded.
 */
class QemuInstanceManager {
public:
    using Target = qemu::Target;
    using LibLoader = qemu::LibraryLoaderIface;

protected:
    LibLoader* m_loader;
    std::vector<std::reference_wrapper<QemuInstance>> m_insts;

public:
    /**
     * @brief Construct a QemuInstanceManager. The manager will use the default
     * library loader provided by libqemu-cxx.
     */
    QemuInstanceManager()
        : m_loader(qemu::get_default_lib_loader())
    {
    }

    /**
     * @brief Construct a QemuInstanceManager by providing a custom library loader
     *
     * @param[in] loader The custom loader
     */
    QemuInstanceManager(LibLoader* loader)
        : m_loader(loader)
    {
    }

    /**
     * @brief Returns a new QEMU instance for target t
     */
    QemuInstance& new_instance(const std::string &n, Target t)
    {
        QemuInstance* ptr = new QemuInstance(n.c_str(), *m_loader, t);
        m_insts.push_back(*ptr);

        return *ptr;
    }
    QemuInstance& new_instance(Target t)
    {
        return new_instance("QemuInstance", t);
    }

    /* Destructor should only be called at the end of the program, if it is called before, then all Qemu instances
 * that it manages will, of course, be destroyed too
 */
    virtual ~QemuInstanceManager()
    {
        while (m_insts.size()) {
            delete &m_insts.back().get();
            m_insts.pop_back();
        }
    }
};
#endif

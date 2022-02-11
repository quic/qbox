/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 GreenSocs SAS
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

#ifndef TESTS_INCLUDE_TEST_CPU_H
#define TESTS_INCLUDE_TEST_CPU_H

#include <keystone/keystone.h>

#include <systemc>
#include <tlm_utils/tlm_quantumkeeper.h>
#include <tlm_utils/simple_target_socket.h>

#include <cci_configuration>
#include <cci/utils/broker.h>

#include <greensocs/base-components/memory.h>
#include <greensocs/base-components/router.h>

#include <greensocs/gsutils/ports/initiator-signal-socket.h>

#include "libqbox/qemu-instance.h"
#include "test/test.h"
#include "test/tester/tester.h"

class CpuTestBenchBase : public TestBench, public CpuTesterCallbackIface
{
public:
    static constexpr uint64_t MEM_ADDR = 0x0;
    static constexpr size_t MEM_SIZE = 256 * 1024;

private:
    ks_arch qemu_to_ks_arch(qemu::Target arch)
    {
        switch (arch) {
        case qemu::Target::AARCH64:
            return KS_ARCH_ARM64;

        default:
            SC_REPORT_FATAL("CpuTestBench", "Unsupported QEMU architecture for Keystone");
            return KS_ARCH_MAX; /* avoid compiler warning */
        }
    }

protected:
    qemu::Target m_arch;

    cci::cci_param<int> p_num_cpu;
    cci::cci_param<int> p_quantum_ns;

    Router<> m_router;
    Memory<> m_mem;

    void set_firmware(const char *assembly)
    {
        ks_engine *ks;
        ks_err err;
        size_t size, count;
        uint8_t *fw;

        err = ks_open(qemu_to_ks_arch(m_arch), KS_MODE_LITTLE_ENDIAN, &ks);

        if (err != KS_ERR_OK) {
            SC_REPORT_FATAL("CpuTestBench", "Unable to initialize keystone\n");
        }

        if (ks_asm(ks, assembly, 0, &fw, &size, &count) != KS_ERR_OK) {
            std::cerr << assembly << "\n";
            TEST_FAIL("Unable to assemble the test firmware\n");
        }

        m_mem.load(fw, size, 0);

        ks_free(fw);
        ks_close(ks);
    }

public:
    CpuTestBenchBase(const sc_core::sc_module_name &n, qemu::Target arch)
        : TestBench(n)
        , m_arch(arch)
        , p_num_cpu("num_cpu", 1, "Number of CPUs to instantiate in the test")
        , p_quantum_ns("quantum_ns", 1000000, "Value of the global TLM-2.0 quantum in ns")
        , m_router("router")
        , m_mem("mem", MEM_SIZE)
    {
        using tlm_utils::tlm_quantumkeeper;

        m_router.add_target(m_mem.socket, MEM_ADDR, MEM_SIZE);

        sc_core::sc_time global_quantum(p_quantum_ns, sc_core::SC_NS);
        tlm_quantumkeeper::set_global_quantum(global_quantum);
    }

    unsigned int get_num_cpus()
    {
        return p_num_cpu;
    }

    void map_target(tlm::tlm_target_socket<> &s, uint64_t addr, uint64_t size)
    {
        m_router.add_target(s, addr, size);
    }

    virtual uint64_t mmio_read(int id, uint64_t addr, size_t len)
    {
        TEST_FAIL("Unexpected CPU read");
    }

    virtual void mmio_write(int id, uint64_t addr, uint64_t data, size_t len)
    {
        TEST_FAIL("Unexpected CPU write");
    }

    virtual bool dmi_request(int id, uint64_t addr, size_t len, tlm::tlm_dmi &ret)
    {
        TEST_FAIL("Unexpected DMI request");
        return false;
    }
};

template <class CPU, class TESTER>
class CpuTestBench : public CpuTestBenchBase
{
protected:
    QemuInstanceManager m_inst_manager;
    QemuInstance &m_inst_a;
    QemuInstance &m_inst_b;

    sc_core::sc_vector<CPU> m_cpus;
    TESTER m_tester;
    bool ab=false;

public:
    CpuTestBench(const sc_core::sc_module_name &n)
        : CpuTestBenchBase(n, CPU::ARCH)
        , m_inst_a(m_inst_manager.new_instance("inst_a",CPU::ARCH))
        , m_inst_b(m_inst_manager.new_instance("inst_b",CPU::ARCH))
        , m_cpus("cpu", p_num_cpu,  [this](const char *n, int i) {
                   ab = !ab;
                   return new CPU(n, ab ? m_inst_a : m_inst_b);
                 })
        , m_tester("tester", *this)
    {
        int i = 0;
        for (CPU &cpu: m_cpus) {
            cpu.p_mp_affinity = i++;
            m_router.add_initiator(cpu.socket);
        }
    }

    void map_irqs_to_cpus(sc_core::sc_vector< InitiatorSignalSocket<bool> > &irqs)
    {
        int i = 0;

        for (auto &cpu: m_cpus) {
            switch (CPU::ARCH) {
            case qemu::Target::AARCH64:
                irqs[i++].bind(cpu.irq_in);
                break;
            default:
                SC_REPORT_FATAL("CpuTestBench", "Don't know how to bind tester IRQs to CPU");
            }
        }
    }

    void map_halt_to_cpus(sc_core::sc_vector<sc_core::sc_out<bool>> &halt){

        int i = 0;

        for (auto &cpu: m_cpus) {
                halt[i++].bind(cpu.halt);
            }
        }
};

#endif

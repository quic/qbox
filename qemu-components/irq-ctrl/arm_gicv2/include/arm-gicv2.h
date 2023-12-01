/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_IRQ_CTRL_ARM_GICV2_H
#define _LIBQBOX_COMPONENTS_IRQ_CTRL_ARM_GICV2_H

#include <systemc>
#include <cci_configuration>

#include <scp/report.h>

#include <module_factory_registery.h>

#include <device.h>
#include <ports/target.h>
#include <ports/qemu-initiator-signal-socket.h>
#include <ports/qemu-target-signal-socket.h>

class arm_gicv2m : public QemuDevice
{
protected:
    cci::cci_param<unsigned int> p_base_spi;
    cci::cci_param<unsigned int> p_num_spis;

public:
    /*
     * Output interrupt lines. Vector are sized with the number of SPIs
     * These cannot be plugged anywhere on the gic: it depends on the
     * base-spi/num-spis parameter.
     */
    sc_core::sc_vector<QemuInitiatorSignalSocket> spi_out;

    QemuTargetSocket<> iface;

    arm_gicv2m(const sc_core::sc_module_name& name, QemuInstance& inst)
        : QemuDevice(name, inst, "arm-gicv2m")
        , p_base_spi("base_spi", 0, "Start index of gic's spis")
        , p_num_spis("num_spis", 0, "Number of gic's spis")
        , spi_out("spi_out", p_num_spis)
        , iface("iface", inst)
    {
    }

    unsigned int get_base_spi() { return p_base_spi; }

    unsigned int get_num_spis() { return p_num_spis; }

    void before_end_of_elaboration()
    {
        QemuDevice::before_end_of_elaboration();

        m_dev.set_prop_int("base-spi", p_base_spi);
        m_dev.set_prop_int("num-spi", p_num_spis);
    }

    void end_of_elaboration()
    {
        QemuDevice::end_of_elaboration();

        qemu::SysBusDevice sbd(m_dev);

        iface.init(sbd, 0);

        /* IRQs out */
        for (int i = 0; i < p_num_spis; i++) {
            spi_out[i].init_sbd(sbd, i);
        }
    }
};

class arm_gicv2 : public QemuDevice
{
public:
    static const uint32_t NUM_PPI = 32;

protected:
    cci::cci_param<unsigned int> p_num_cpu;
    cci::cci_param<unsigned int> p_num_spi;
    cci::cci_param<unsigned int> p_revision;
    cci::cci_param<bool> p_has_virt_extensions;
    cci::cci_param<bool> p_has_security_extensions;
    cci::cci_param<unsigned int> p_num_prio_bits;
    cci::cci_param<bool> p_has_msi_support;

public:
    arm_gicv2m* m_gicv2m;

    QemuTargetSocket<> dist_iface;
    QemuTargetSocket<> cpu_iface;
    QemuTargetSocket<> virt_iface;
    QemuTargetSocket<> vcpu_iface;
    QemuTargetSocket<>::TlmTargetSocket v2m_iface;

    /* Shared peripheral interrupts: sized with the p_num_spi parameter */
    sc_core::sc_vector<QemuTargetSignalSocket> spi_in;

    /*
     * Private peripheral interrupts: 32 per CPUs, the outer vector is sized with the
     * number of CPUs.
     */
    sc_core::sc_vector<sc_core::sc_vector<QemuTargetSignalSocket> > ppi_in;

    /* Output interrupt lines. Vector are sized with the number of CPUs */
    sc_core::sc_vector<QemuInitiatorSignalSocket> irq_out;
    sc_core::sc_vector<QemuInitiatorSignalSocket> fiq_out;
    sc_core::sc_vector<QemuInitiatorSignalSocket> virq_out;
    sc_core::sc_vector<QemuInitiatorSignalSocket> vfiq_out;
    sc_core::sc_vector<QemuInitiatorSignalSocket> maintenance_out;

    arm_gicv2(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : arm_gicv2(name, *(dynamic_cast<QemuInstance*>(o)))
    {
    }
    arm_gicv2(const sc_core::sc_module_name& name, QemuInstance& inst)
        : QemuDevice(name, inst, "arm_gic")
        , p_num_cpu("num_cpu", 0, "Number of CPU interfaces")
        , p_num_spi("num_spi", 0, "Number of shared peripheral interrupts")
        , p_revision("revision", 2, "Revision of the GIC (1 -> v1, 2 -> v2, 0 -> 11MPCore)")
        , p_has_virt_extensions("has_virt_extensions", false,
                                "Enable virtualization extensions "
                                "(v2 only)")
        , p_has_security_extensions("has_security_extensions", false,
                                    "Enable security extensions "
                                    "(v1 and v2 only)")
        , p_num_prio_bits("num_prio_bits", 8, "Number of priority bits implemented by this GIC")
        , p_has_msi_support("has_msi_support", false, "Enable gicv2m extension to support MSI")
        , m_gicv2m(NULL)
        , dist_iface("dist_iface", inst)
        , cpu_iface("cpu_iface", inst)
        , virt_iface("virt_iface", inst)
        , vcpu_iface("vcpu_iface", inst)
        , v2m_iface("v2m_iface")
        , spi_in("spi_in", p_num_spi)
        , ppi_in("ppi_in_cpu", p_num_cpu,
                 [](const char* n, size_t i) { return new sc_core::sc_vector<QemuTargetSignalSocket>(n, NUM_PPI); })
        , irq_out("irq_out", p_num_cpu)
        , fiq_out("fiq_out", p_num_cpu)
        , virq_out("virq_out", p_num_cpu)
        , vfiq_out("vfiq_out", p_num_cpu)
        , maintenance_out("maintenance_out", p_num_cpu)
    {
        if (p_has_msi_support) {
            m_gicv2m = new arm_gicv2m("v2m", inst);

            // Hierarchical bind our v2m_iface to our child interface
            v2m_iface.bind(m_gicv2m->iface);

            // Connect spis
            unsigned int v2m_num_spis = m_gicv2m->get_num_spis();
            unsigned int v2m_base_spi = m_gicv2m->get_base_spi();
            if (v2m_num_spis + v2m_base_spi > p_num_spi) {
                SCP_FATAL(SCMOD) << "Gicv2 does not have enough spis for v2m config";
            }
            for (unsigned int i = 0; i < v2m_num_spis; i++) {
                m_gicv2m->spi_out[i].bind(spi_in[i + v2m_base_spi]);
            }
        }
    }

    ~arm_gicv2()
    {
        if (m_gicv2m) {
            delete m_gicv2m;
        }
    }

    void before_end_of_elaboration()
    {
        QemuDevice::before_end_of_elaboration();

        m_dev.set_prop_int("num-cpu", p_num_cpu);
        m_dev.set_prop_int("num-irq", p_num_spi + NUM_PPI);
        m_dev.set_prop_int("revision", p_revision);
        m_dev.set_prop_bool("has-virtualization-extensions", p_has_virt_extensions);
        m_dev.set_prop_bool("has-security-extensions", p_has_security_extensions);
        m_dev.set_prop_int("num-priority-bits", p_num_prio_bits);
    }

    void end_of_elaboration()
    {
        QemuDevice::set_sysbus_as_parent_bus();
        QemuDevice::end_of_elaboration();

        qemu::SysBusDevice sbd(m_dev);
        int cpu, i;

        dist_iface.init(sbd, 0);
        cpu_iface.init(sbd, 1);

        if (p_has_virt_extensions) {
            virt_iface.init(sbd, 2);
            vcpu_iface.init(sbd, 3);
        }

        /* SPIs in */
        for (i = 0; i < p_num_spi; i++) {
            spi_in[i].init(m_dev, i);
        }

        /* PPIs in */
        for (cpu = 0; cpu < p_num_cpu; cpu++) {
            for (i = 0; i < NUM_PPI; i++) {
                int ppi_idx = p_num_spi + cpu * NUM_PPI + i;
                ppi_in[cpu][i].init(m_dev, ppi_idx);
            }
        }

        /* Output lines */
        for (cpu = 0; cpu < p_num_cpu; cpu++) {
            irq_out[cpu].init_sbd(sbd, p_num_cpu * 0 + cpu);
            fiq_out[cpu].init_sbd(sbd, p_num_cpu * 1 + cpu);
            virq_out[cpu].init_sbd(sbd, p_num_cpu * 2 + cpu);
            vfiq_out[cpu].init_sbd(sbd, p_num_cpu * 3 + cpu);

            if (p_has_virt_extensions) {
                maintenance_out[cpu].init_sbd(sbd, p_num_cpu * 4 + cpu);
            }
        }
    }
};

extern "C" void module_register();

#endif

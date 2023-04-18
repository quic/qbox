/*
 *  This file is part of libqbox
 *  Copyright (c) 2020 GreenSocs
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

#ifndef _LIBQBOX_COMPONENTS_IRQ_CTRL_ARM_GICV3_H
#define _LIBQBOX_COMPONENTS_IRQ_CTRL_ARM_GICV3_H

#include <sstream>

#include <systemc>
#include <cci_configuration>

#include "libqbox/components/device.h"
#include "libqbox/ports/target.h"
#include "libqbox/ports/initiator-signal-socket.h"
#include "libqbox/ports/target-signal-socket.h"
#include "libqbox/sc-qemu-instance.h"

class QemuArmGicv3 : public QemuDevice
{
public:
    static const uint32_t NUM_PPI = 32;

protected:
    cci::cci_param<unsigned int> p_num_cpu;
    cci::cci_param<unsigned int> p_num_spi;
    cci::cci_param<unsigned int> p_revision;
    cci::cci_param<std::vector<unsigned int> > p_redist_region;
    cci::cci_param<bool> p_has_security_extensions;

public:
    QemuTargetSocket<> dist_iface;
    sc_core::sc_vector<QemuTargetSocket<> > redist_iface;

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
    QemuArmGicv3(const sc_core::sc_module_name& name, sc_core::sc_object* o)
        : QemuArmGicv3(name, dynamic_cast<SC_QemuInstance*>(o)->getQemuInst())
        {
        }
    QemuArmGicv3(const sc_core::sc_module_name& name, QemuInstance& inst, unsigned num_cpus = 0)
        : QemuDevice(name, inst, "arm-gicv3")
        , p_num_cpu("num_cpus", num_cpus, "Number of CPU interfaces")
        , p_num_spi("num_spi", 0, "Number of shared peripheral interrupts")
        , p_revision("revision", 3, "Revision of the GIC (3 -> v3, the only supported revision)")
        // , p_redist_region("redist_region", std::vector<unsigned int>({}),
        //                   "Redistributor regions configuration")
        , p_redist_region("redist_region", std::vector<unsigned int>(gs::cci_get_vector<unsigned int>(std::string(sc_module::name())+".redist_region")),
                                                            "Redistributor regions configuration")
        , p_has_security_extensions("has_security_extensions", false, "Enable security extensions")
        , dist_iface("dist_iface", inst)
        , redist_iface("redist_iface", p_redist_region.get_value().size(),
                       [&inst](const char* n, int i) { return new QemuTargetSocket<>(n, inst); })
        , spi_in("spi_in", p_num_spi)
        , ppi_in("ppi_in_cpu", p_num_cpu,
                 [](const char* n, size_t i) {
                     return new sc_core::sc_vector<QemuTargetSignalSocket>(n, NUM_PPI);
                 })
        , irq_out("irq_out", p_num_cpu)
        , fiq_out("fiq_out", p_num_cpu)
        , virq_out("virq_out", p_num_cpu)
        , vfiq_out("vfiq_out", p_num_cpu) {}

    void before_end_of_elaboration() {
        QemuDevice::before_end_of_elaboration();
        int i;

        m_dev.set_prop_int("num-cpu", p_num_cpu);
        m_dev.set_prop_int("num-irq", p_num_spi + NUM_PPI);
        m_dev.set_prop_int("revision", p_revision);
        m_dev.set_prop_bool("has-security-extensions", p_has_security_extensions);

        m_dev.set_prop_int("len-redist-region-count", p_redist_region.get_value().size());
        for (i = 0; i < p_redist_region.get_value().size(); i++) {
            std::stringstream ss;

            ss << "redist-region-count[" << i << "]";
            m_dev.set_prop_int(ss.str().c_str(), p_redist_region.get_value()[i]);
        }
    }

    void end_of_elaboration() {
        QemuDevice::set_sysbus_as_parent_bus();
        QemuDevice::end_of_elaboration();

        qemu::SysBusDevice sbd(m_dev);
        int cpu, i;

        dist_iface.init(sbd, 0);

        for (i = 0; i < p_redist_region.get_value().size(); i++) {
            redist_iface[i].init(sbd, 1 + i);
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
        }
    }
};

#endif

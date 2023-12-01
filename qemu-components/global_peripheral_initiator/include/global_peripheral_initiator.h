/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_COMPONENTS_GOLBAL_PERIPHERAL_INITIATOR_H
#define _LIBQBOX_COMPONENTS_GOLBAL_PERIPHERAL_INITIATOR_H

#include <ports/initiator.h>
#include <device.h>
#include <qemu-instance.h>
#include <module_factory_registery.h>

class global_peripheral_initiator : public QemuInitiatorIface, public sc_core::sc_module
{
public:
    // QemuInitiatorIface functions
    using TlmPayload = tlm::tlm_generic_payload;
    virtual void initiator_customize_tlm_payload(TlmPayload& payload) override {}
    virtual void initiator_tidy_tlm_payload(TlmPayload& payload) override {}
    virtual sc_core::sc_time initiator_get_local_time() override { return sc_core::sc_time_stamp(); }
    virtual void initiator_set_local_time(const sc_core::sc_time&) override {}

    QemuInitiatorSocket<> m_initiator;
    global_peripheral_initiator(const sc_core::sc_module_name& name, sc_core::sc_object* o, sc_core::sc_object* t)
        : global_peripheral_initiator(name, *(dynamic_cast<QemuInstance*>(o)), *(dynamic_cast<QemuDevice*>(t)))
    {
    }
    global_peripheral_initiator(const sc_core::sc_module_name& nm, QemuInstance& inst, QemuDevice& owner)
        : m_initiator("global_initiator", *this, inst), m_owner(owner)
    {
    }

    virtual void before_end_of_elaboration() override
    {
        qemu::Device dev = m_owner.get_qemu_dev();
        m_initiator.init_global(dev);
    }

    virtual void initiator_async_run(qemu::Cpu::AsyncJobFn job) override {}

private:
    QemuDevice& m_owner;
};

extern "C" void module_register();
#endif //_LIBQBOX_COMPONENTS_GOLBAL_PERIPHERAL_INITIATOR_H

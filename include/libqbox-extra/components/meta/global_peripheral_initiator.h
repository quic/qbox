/*
 *  This file is part of libqbox
 *  Copyright (c) 2021 Greensocs
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

#ifndef _LIBQBOX_COMPONENTS_GOLBAL_PERIPHERAL_INITIATOR_H
#define _LIBQBOX_COMPONENTS_GOLBAL_PERIPHERAL_INITIATOR_H

#include "libqbox/ports/initiator.h"
#include "libqbox/components/device.h"

class GlobalPeripheralInitiator : public QemuInitiatorIface, public sc_core::sc_module
{
public:
    // QemuInitiatorIface functions
    using TlmPayload = tlm::tlm_generic_payload;
    virtual void initiator_customize_tlm_payload(TlmPayload& payload) override {}
    virtual void initiator_tidy_tlm_payload(TlmPayload& payload) override {}
    virtual sc_core::sc_time initiator_get_local_time() override {
        return sc_core::sc_time_stamp();
    }
    virtual void initiator_set_local_time(const sc_core::sc_time&) override {}

    QemuInitiatorSocket<> m_initiator;

    GlobalPeripheralInitiator(const sc_core::sc_module_name& nm, QemuInstance& inst,
                              QemuDevice& owner)
        : m_initiator("global_initiator", *this, inst), m_owner(owner) {}

    virtual void before_end_of_elaboration() override {
        qemu::Device dev = m_owner.get_qemu_dev();
        m_initiator.init_global(dev);
    }

private:
    QemuDevice& m_owner;
};

#endif //_LIBQBOX_COMPONENTS_GOLBAL_PERIPHERAL_INITIATOR_H

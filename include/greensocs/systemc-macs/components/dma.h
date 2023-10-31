/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once
#include <greensocs/gsutils/tlm_sockets_buswidth.h>

class Dma : public sc_core::sc_module
{
public:
    tlm_utils::simple_initiator_socket<Dma, DEFAULT_TLM_BUSWIDTH> socket;

    tlm::tlm_dmi dmi_data;
    bool dmi_valid;

    Dma(const sc_core::sc_module_name& name): sc_core::sc_module(name), socket("socket") { dmi_valid = false; }

    ~Dma() {}

    void bus_do(uint64_t addr, uint8_t* data, uint32_t size, bool is_write)
    {
        tlm::tlm_generic_payload trans;

        if (dmi_valid && addr >= dmi_data.get_start_address() && (addr + size) < dmi_data.get_end_address()) {
            if (is_write && dmi_data.is_write_allowed()) {
                uint64_t off = addr - dmi_data.get_start_address();
                unsigned char* ptr = dmi_data.get_dmi_ptr();
                memcpy(ptr + off, data, size);
                return;
            } else if (!is_write && dmi_data.is_read_allowed()) {
                uint64_t off = addr - dmi_data.get_start_address();
                unsigned char* ptr = dmi_data.get_dmi_ptr();
                memcpy(data, ptr + off, size);
                return;
            }
        }

        trans.set_command(is_write ? tlm::TLM_WRITE_COMMAND : tlm::TLM_READ_COMMAND);
        trans.set_address(addr);
        trans.set_data_ptr(reinterpret_cast<unsigned char*>(data));
        trans.set_data_length(size);
        trans.set_streaming_width(size);
        trans.set_byte_enable_length(0);
        trans.set_dmi_allowed(false);
        trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        sc_core::sc_time delay(sc_core::SC_ZERO_TIME);
        socket->b_transport(trans, delay);

        if (trans.is_dmi_allowed()) {
            dmi_valid = socket->get_direct_mem_ptr(trans, dmi_data);
        }
    }

    void bus_write(uint64_t addr, uint8_t* data, uint32_t size) { bus_do(addr, data, size, 1); }

    void bus_read(uint64_t addr, uint8_t* data, uint32_t size) { bus_do(addr, data, size, 0); }
};

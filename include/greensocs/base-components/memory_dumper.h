/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GREENSOCS_BASE_COMPONENTS_MemoryDumper_DUMPER_H
#define _GREENSOCS_BASE_COMPONENTS_MemoryDumper_DUMPER_H

#include <cci_configuration>
#include <systemc>
#include <tlm>
#include <scp/report.h>

#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include "greensocs/base-components/memory.h"
#include "greensocs/gsutils/cciutils.h"
#include <greensocs/gsutils/module_factory_registery.h>
#include <greensocs/gsutils/tlm_sockets_buswidth.h>

namespace gs {

template <typename T>
std::vector<T>& join(std::vector<T>& vector1, const std::vector<T>& vector2)
{
    vector1.insert(vector1.end(), vector2.begin(), vector2.end());
    return vector1;
}

template <typename T>
std::vector<std::string> find_object_of_type(sc_core::sc_object* m = NULL)
{
    std::vector<std::string> list;

    if (dynamic_cast<T*>(m)) {
        list.push_back(std::string(m->name()));
        return list;
    }

    std::vector<sc_core::sc_object*> children;
    if (m) {
        children = m->get_child_objects();
    } else {
        children = sc_core::sc_get_top_level_objects();
    }
    for (auto c : children) {
        list = join<std::string>(list, find_object_of_type<T>(c));
    }
    return list;
}

/**
 * @class MemoryDumperDumper
 *
 * @brief A device that finds Memory components through the system and trys to
 * dump their MemoryDumper
 */

template <unsigned int BUSWIDTH = DEFAULT_TLM_BUSWIDTH>
class memory_dumper : public sc_core::sc_module
{
    cci::cci_broker_handle m_broker;

    cci::cci_param<bool> p_dump;
    cci::cci_param<std::string> p_outfile;

protected:
#define LINESIZE 16
    void dump()
    {
        for (std::string m : gs::find_object_of_type<memory<BUSWIDTH>>()) {
            uint64_t addr = gs::cci_get<uint64_t>(m_broker, m + ".target_socket.address");
            uint64_t size = gs::cci_get<uint64_t>(m_broker, m + ".target_socket.size");
            tlm::tlm_generic_payload trans;
            std::stringstream fnamestr;
            fnamestr << m << ".0x" << std::hex << addr << "-0x" << (addr + size) << "." << p_outfile.get_value();
            std::string fname = fnamestr.str();

            FILE* out = fopen(fname.c_str(), "wb");

            uint8_t data[LINESIZE];
            for (uint64_t offset = 0; offset < size;) {
                int rsize = LINESIZE;
                if (offset + rsize >= size) {
                    rsize = size - offset;
                }
                trans.set_command(tlm::TLM_READ_COMMAND);
                trans.set_address(addr + offset);
                trans.set_data_ptr(data);
                trans.set_data_length(rsize);
                trans.set_streaming_width(rsize);
                trans.set_byte_enable_length(0);
                tlm::tlm_dmi dmi;
                if (!initiator_socket->get_direct_mem_ptr(trans, dmi)) {
                    std::stringstream info;
                    SCP_WARN(SCMOD) << "loading data (no DMI) from memory @ "
                                    << "0x" << std::hex << addr + offset;
                    break;
                }
                uint64_t size = (dmi.get_end_address() - dmi.get_start_address()) + 1;
                uint8_t* ptr = dmi.get_dmi_ptr();

                if (fwrite(ptr, size, 1, out) != 1) {
                    SCP_WARN(SCMOD) << "saving data to file " << fname;
                }
                offset += size;
            }
            fclose(out);
        }
    }

    void b_transport(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay)
    {
        unsigned int len = txn.get_data_length();
        unsigned char* ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();
        if (addr == 0x0) {
            dump();
        }
        txn.set_response_status(tlm::TLM_OK_RESPONSE);
    }

public:
    tlm_utils::simple_initiator_socket<memory_dumper<BUSWIDTH>, BUSWIDTH> initiator_socket;
    tlm_utils::simple_target_socket<memory_dumper<BUSWIDTH>, BUSWIDTH> target_socket;

    memory_dumper(sc_core::sc_module_name name)
        : m_broker(cci::cci_get_broker())
        , p_dump("MemoryDumper_trigger", false)
        , p_outfile("outfile", "dumpfile")
        , initiator_socket("initiator_socket")
        , target_socket("target_socket")
    {
        SCP_DEBUG(SCMOD) << "MemoryDumper constructor";
        p_dump.register_post_write_callback([this](auto ev) { this->dump(); });
        target_socket.register_b_transport(this, &memory_dumper::b_transport);
    }

    memory_dumper() = delete;
    memory_dumper(const memory_dumper&) = delete;

    ~memory_dumper() {}
};

void memorydumper_tgr_helper()
{
    for (std::string m : gs::find_object_of_type<memory_dumper<>>()) {
        auto ph = cci::cci_param_typed_handle<bool>(
            cci::cci_get_broker().get_param_handle(std::string(m) + ".MemoryDumper_trigger"));
        ph.set_value(true);
    }
}

} // namespace gs

extern "C" void module_register();
#endif

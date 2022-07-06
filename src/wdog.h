#pragma once

#include <fstream>

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>

template <unsigned int BUSWIDTH = 32>
class WDog : public sc_core::sc_module {

protected:
    uint32_t regs[0x100];
    void b_transport(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay)
    {
        unsigned int len = txn.get_data_length();
        unsigned char* ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();
        unsigned char* byt = txn.get_byte_enable_ptr();
        unsigned int bel = txn.get_byte_enable_length();

        if (txn.get_streaming_width() < len) {
            SC_REPORT_ERROR("wdog", "not supported.\n");
        }

        std::string access = "";

        switch (txn.get_command()) {
        case tlm::TLM_READ_COMMAND:
            *(uint32_t*)ptr = regs[addr];
            access = "Read";
            break;
        case tlm::TLM_WRITE_COMMAND:
            regs[addr] = (*(uint32_t*)ptr);
            access = "Write";
            break;
        default:
            break;
        }
        if ((addr & 0xff) != 0x0) {
            std::stringstream info;
            info << name() << " : " << access << " access to address "
                 << "0x" << std::hex << addr << " value 0x" << (*(uint32_t*)ptr) << " (reg: 0x" << regs[addr] << ")";
            SC_REPORT_INFO("WDog", info.str().c_str());
        }
        txn.set_response_status(tlm::TLM_OK_RESPONSE);

        txn.set_dmi_allowed(false);
    }

public:
    tlm_utils::simple_target_socket<WDog, BUSWIDTH> socket;

    WDog(sc_core::sc_module_name name)
        : socket("socket")
    {
        socket.register_b_transport(this, &WDog::b_transport);
        regs[0xc] = 0xdeadbeef;
    }
};
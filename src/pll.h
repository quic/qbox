#pragma once

#include <fstream>

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>

#include <scp/report.h>

template <unsigned int BUSWIDTH = 32>
class pll : public sc_core::sc_module
{
    uint32_t regs[0x10000];

protected:
    void b_transport(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay) {
        unsigned int len = txn.get_data_length();
        unsigned char* ptr = txn.get_data_ptr();
        sc_dt::uint64 addr = txn.get_address();

        if (txn.get_streaming_width() < len) {
            SCP_ERR(SCMOD) << "not supported.";
        }

        regs[0x0] |= 0b10000000000000000011100000000000;
        regs[0x4000] = 0;

        switch (txn.get_command()) {
        case tlm::TLM_READ_COMMAND:
            *(uint32_t*)ptr = regs[addr];
            break;
        case tlm::TLM_WRITE_COMMAND:
            regs[addr] = (*(uint32_t*)ptr);
            if ((addr & 0xffff) == 0x1024) {
                regs[0x1020] |= 0b100;
            }
            if (((addr & 0xffff) == 0x1020) && (regs[0x1020] & 0b1)) {
                regs[0x1020] &= ~0b101;
            }
            break;
        default:
            break;
        }

        txn.set_response_status(tlm::TLM_OK_RESPONSE);

        txn.set_dmi_allowed(false);
    }

public:
    tlm_utils::simple_target_socket<pll, BUSWIDTH> socket;

    pll(sc_core::sc_module_name name): socket("socket") {
        socket.register_b_transport(this, &pll::b_transport);
    }
};

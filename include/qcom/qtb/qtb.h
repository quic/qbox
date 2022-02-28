/*
 *
 *  copywrite Qualcomm 2021
 */

#ifndef _QUALCOMM_COMPONENTS_QTB
#define _QUALCOMM_COMPONENTS_QTB

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

template <unsigned int BUSWIDTH = 32>
class qtb : public sc_core::sc_module {
private:
    enum {
        QTB_OVR_ECATS_INFLD0 = 0x430,
     QTB_OVR_ECATS_INFLD1 = 0x438,

     QTB_OVR_ECATS_INFLD2 = 0x440,
     QTB_OVR_ECATS_TRIGGER = 0x448,
     QTB_OVR_ECATS_STATUS = 0x450,
     QTB_OVR_ECATS_OUTFLD0 = 0x458,
     QTB_OVR_ECATS_OUTFLD1 = 0x460
    };
    uint64_t infld[3];
    uint64_t outfld[2];
    bool triggered = false;

public:
    tlm_utils::simple_initiator_socket<qtb<BUSWIDTH>> initiator_socket;
    tlm_utils::simple_target_socket<qtb, BUSWIDTH> target_socket;
    tlm_utils::simple_target_socket<qtb, BUSWIDTH> control_socket;

private:
    void b_transport_control(tlm::tlm_generic_payload& trans,
        sc_core::sc_time& delay)
    {
        unsigned char* ptr = trans.get_data_ptr();
        sc_dt::uint64 addr = trans.get_address();
        switch (trans.get_command()) {
        case tlm::TLM_WRITE_COMMAND:
            switch (addr) {
            case QTB_OVR_ECATS_INFLD0:
                infld[0] = *ptr;
                break;
            case QTB_OVR_ECATS_INFLD1:
                infld[0] = *ptr;
                break;
            case QTB_OVR_ECATS_INFLD2:
                infld[0] = *ptr;
                break;
            case QTB_OVR_ECATS_TRIGGER:
                if (*ptr) {
                    unsigned char data[8];
                    sc_core::sc_time time;
                    tlm::tlm_generic_payload trans;

                    trans.set_command(tlm::TLM_WRITE_COMMAND);
                    trans.set_address(infld[2]);
                    trans.set_data_ptr(&data[0]);
                    trans.set_data_length(sizeof(data));
                    trans.set_streaming_width(sizeof(data));
                    trans.set_byte_enable_length(0);
                    initiator_socket->b_transport(trans, time);
                } else {
                    triggered = false;
                }
                break;
            default:
                SC_REPORT_FATAL("qtb", "Unknown write");
            }
            break;
        case tlm::TLM_READ_COMMAND:
            switch (addr) {
            case QTB_OVR_ECATS_STATUS:
                *ptr = triggered;
                break;
            case QTB_OVR_ECATS_OUTFLD0:
                *ptr = outfld[0];
                break;
            case QTB_OVR_ECATS_OUTFLD1:
                *ptr = outfld[1];
                break;
            default:
                SC_REPORT_FATAL("qtb", "Unknown read");
                break;
            }
        default:
            SC_REPORT_FATAL("qtb", "Unknown tlm command");
        }
    }

    void b_transport(tlm::tlm_generic_payload& trans,
        sc_core::sc_time& delay)
    {
        triggered = true;
        sc_dt::uint64 addr = trans.get_address();
        outfld[0] = addr << 12;
        outfld[1] = infld[1]; // hack.
    }

public:
    explicit qtb(const sc_core::sc_module_name& nm)
        : sc_core::sc_module(nm)
        , initiator_socket("initiator_socket")
        , target_socket("target_socket")
        , control_socket("control_socket")        
    {
        control_socket.register_b_transport(this, &qtb::b_transport_control);
        target_socket.register_b_transport(this, &qtb::b_transport);
    }
    qtb() = delete;
    qtb(const qtb&) = delete;
    ~qtb() { }
};

#endif
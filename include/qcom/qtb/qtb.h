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

#include <scp/report.h>

template <unsigned int BUSWIDTH = 32>
class qtb : public sc_core::sc_module {
private:
    enum {
        QTB_OVR_ECATS_INFLD0 = 0x0, // APPS_SMMU_CLIENT_DEBUG_SID_HALT
        QTB_OVR_ECATS_INFLD1 = 0x8, // APPS_SMMU_CLIENT_DEBUG_VA_ADDR
        QTB_OVR_ECATS_INFLD2 = 0x10, // APPS_SMMU_CLIENT_DEBUG_SSD_INDEX
        QTB_OVR_ECATS_TRIGGER = 0x18, // APPS_SMMU_CLIENT_DEBUG_TRANSCATION_TRIGG
        QTB_OVR_ECATS_STATUS = 0x20, // APPS_SMMU_CLIENT_DEBUG_STATUS_HALT_ACK
        QTB_OVR_ECATS_OUTFLD0 = 0x28, // APPS_SMMU_CLIENT_DEBUG_PAR
        QTB_OVR_ECATS_OUTFLD1 = 0x2C, // APPS_SMMU_CLIENT_DEBUG_PAR
    };
    uint32_t infld[3];
    uint32_t outfld[2];
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

        trans.set_response_status(tlm::TLM_OK_RESPONSE);

        switch (trans.get_command()) {
        case tlm::TLM_WRITE_COMMAND:
            switch (addr & 0xff) {
            case QTB_OVR_ECATS_INFLD0:
                memcpy(&infld[0], ptr, 4);
                break;
            case QTB_OVR_ECATS_INFLD1:
                memcpy(&infld[1], ptr, 4);
                break;
            case QTB_OVR_ECATS_INFLD2:
                memcpy(&infld[2], ptr, 4);
                break;
            case QTB_OVR_ECATS_TRIGGER:
                if (*ptr) {
                    unsigned char data[8];
                    sc_core::sc_time time;
                    tlm::tlm_generic_payload trans;

                    trans.set_command(tlm::TLM_WRITE_COMMAND);
                    trans.set_address(infld[1]);
                    trans.set_data_ptr(&data[0]);
                    trans.set_data_length(sizeof(data));
                    trans.set_streaming_width(sizeof(data));
                    trans.set_byte_enable_length(0);

                    //                    set SID
                    SCP_INFO(SCMOD) << "Sending to TBU: " << std::hex << infld[2];
                    initiator_socket->b_transport(trans, time);
                } else {
                    triggered = false;
                }
                break;
            default:
                trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
                SCP_FATAL(SCMOD) << "Unknown write";
            }
            break;
        case tlm::TLM_READ_COMMAND:
            switch (addr & 0xff) {
            case QTB_OVR_ECATS_STATUS:
                memcpy(ptr, &triggered, 4);
                break;
            case QTB_OVR_ECATS_OUTFLD0:
                memcpy(ptr, &outfld[0], 4);
                break;
            case QTB_OVR_ECATS_OUTFLD1:
                memcpy(ptr, &outfld[1], 4);
                break;
            default:
                trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
                SCP_FATAL(SCMOD) << "Unknown read";
                break;
            }
            break;
        default:
            SCP_FATAL(SCMOD) << "Unknown tlm command";
        }
    }

    void b_transport(tlm::tlm_generic_payload& trans,
        sc_core::sc_time& delay)
    {
        triggered = true;
        sc_dt::uint64 addr = trans.get_address();
        SCP_INFO(SCMOD) << "received from TBU: " << std::hex << addr;
        outfld[0] = addr << 12;
        outfld[1] = addr >> (32 - 12);
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
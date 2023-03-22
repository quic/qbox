//==============================================================================
//
// Copyright (c) 2022 Qualcomm Innovation Center, Inc.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted (subject to the limitations in the
// disclaimer below) provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above
//      copyright notice, this list of conditions and the following
//      disclaimer in the documentation and/or other materials provided
//      with the distribution.
//
//    * Neither the name Qualcomm Innovation Center nor the names of its
//      contributors may be used to endorse or promote products derived
//      from this software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
// GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
// HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//==============================================================================

#ifndef __QUP_UART__
#define __QUP_UART__

#include <mutex>
#include <queue>

#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"
#include <systemc>

#include "greensocs/systemc-uarts/backends/char-backend.h"

#include <greensocs/gsutils/ports/initiator-signal-socket.h>
#include <greensocs/gsutils/ports/target-signal-socket.h>

#include "qupv3_regs.h"
#include <unordered_map>
#include <scp/report.h>

using namespace std;

void qupv3_write(uint64_t offset, uint32_t value);
uint32_t qupv3_read(uint64_t offset);
static int qupv3_can_receive(void* opaque);
static void qupv3_receive(void* opaque, const uint8_t* buf, int size);

class QUPv3;
typedef QUPv3 uart_qup;

class QUPv3 : public sc_core::sc_module
{
    SCP_LOGGER();

public:
    CharBackend* chr;

    tlm_utils::simple_target_socket<QUPv3> socket;

    InitiatorSignalSocket<bool> irq;

#ifdef QUP_UART_TEST
    TargetSignalSocket<bool> dummy_target;
#endif

    sc_core::sc_event update_event;

    /* QUPv3 : <address, data> Handle entity to used in Software */
    /* QUPv3 : Handle entity to used in model */
    unordered_map<uint32_t, uint32_t> qupv3_handle = {
        { GENI_M_CMD_0, 0x0 },        { GENI_M_IRQ_STATUS, 0x0 },    { GENI_M_IRQ_CLEAR, 0x0 },
        { GENI_TX_FIFO_0, 0x0 },      { GENI_TX_FIFO_STATUS, 0x0 },  { UART_TX_TRANS_LEN, 0x0 },
        { GENI_RX_FIFO_0, 0x0 },      { GENI_S_IRQ_STATUS, 0x0 },    { GENI_FW_REVISION_RO, 0x2ff },
        { GENI_RX_FIFO_STATUS, 0x0 }, { SE_HW_PARAM_0, 0x20102864 }, { SE_HW_PARAM_1, 0x20204800 },
        { UART_MANUAL_RFR, 0x0 },
    };

#ifdef QUP_UART_TEST
    SC_HAS_PROCESS(QUPv3);
    QUPv3(sc_core::sc_module_name name)
        : irq("irq")
        , dummy_target("dummy_target")
#else
    SC_HAS_PROCESS(QUPv3);
    QUPv3(sc_core::sc_module_name name)
        : irq("irq")
#endif
    {
        SCP_DEBUG(())("Constructor");
        chr = NULL;

        socket.register_b_transport(this, &QUPv3::b_transport);

        SC_METHOD(qupv3_update_sysc);
        sensitive << update_event;
    }

    void set_backend(CharBackend* backend) {
        chr = backend;
        chr->register_receive(this, qupv3_receive, qupv3_can_receive);
    }

    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
        unsigned char* ptr = trans.get_data_ptr();
        uint64_t addr      = trans.get_address();
        uint32_t ptr_data  = 0;
        /* call function to dump all the registers */
        trans.set_dmi_allowed(false);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);

        switch (trans.get_command()) {
        case tlm::TLM_WRITE_COMMAND:
            memcpy(&ptr_data, ptr, sizeof(uint32_t));
            qupv3_write(addr, ptr_data);
            break;
        case tlm::TLM_READ_COMMAND:
            ptr_data = qupv3_read(addr);
            memcpy(ptr, &ptr_data, sizeof(uint32_t));

            break;
        default:
            break;
        }
    }

    void qupv3_update() { update_event.notify(); }

    int irq_level() {
        return (qupv3_handle[GENI_M_IRQ_STATUS] & M_CMD_DONE) ||
               (qupv3_handle[GENI_S_IRQ_STATUS] & (RX_FIFO_LAST));
    }
    void qupv3_update_sysc() {
        SCP_DEBUG(())("Writing IRQ = 0x{:x}", irq_level());
        irq->write(irq_level());

        /* If the s/w has requested a clear, but there is still an outstanding RX, then reset and
         * resent the IRQ*/
        if (!irq_level() && qupv3_handle[GENI_RX_FIFO_STATUS]) {
            qupv3_handle[GENI_S_IRQ_STATUS] |= (RX_FIFO_LAST);
            SCP_DEBUG(())("Writing IRQ = 0x{:x}", irq_level());
            irq->write(irq_level());
        }
    }

    uint32_t qupv3_read(uint64_t offset) {
        uint32_t c;
        uint32_t r;

        switch (offset) {
        case GENI_M_IRQ_STATUS:
            /* Check for this two bits TX_FIFO_WR_ERR and M_CMD_DONE */
            r = qupv3_handle[GENI_M_IRQ_STATUS];
            break;
        case GENI_TX_FIFO_STATUS:
            SCP_DEBUG(()) << hex << "Ignore for now Addr(GENI_TX_FIFO_STATUS)";
            r = 0;
            break;
        case GENI_S_IRQ_STATUS:
            SCP_DEBUG(()) << hex << "Ignore for now Addr(GENI_S_IRQ_STATUS)";
            r = qupv3_handle[GENI_S_IRQ_STATUS];
            break;
        case GENI_RX_FIFO_0:
            /* Return Rx FIFO data which we have read using stdout via backend */
            r = qupv3_handle[GENI_RX_FIFO_0];
            qupv3_handle[GENI_S_IRQ_STATUS] &= ~RX_FIFO_LAST;
            {
                int n = (qupv3_handle[GENI_RX_FIFO_STATUS] >> 28) & (GENI_RX_FIFO_MAX - 1);
                if (n)
                    n--;
                if (n) {
                    qupv3_handle[GENI_RX_FIFO_STATUS] = (RX_LAST) | (n << 28);
                    for (int i = 0; i < n; i++) {
                        qupv3_handle[GENI_RX_FIFO_0] = qupv3_handle[GENI_RX_FIFO_0 + 1];
                    }
                } else {
                    qupv3_handle[GENI_RX_FIFO_STATUS] = 0x0;
                }
            }
            qupv3_update();
            break;
        case GENI_FW_REVISION_RO:
            r = qupv3_handle[GENI_FW_REVISION_RO];
            break;
        case GENI_RX_FIFO_STATUS:
            r = qupv3_handle[GENI_RX_FIFO_STATUS];
            //            qupv3_handle[GENI_RX_FIFO_STATUS] = 0x0;
            break;
        case SE_HW_PARAM_0:
            r = qupv3_handle[SE_HW_PARAM_0];
            SCP_DEBUG(()) << hex << "Unhandled READ at offset :" << offset;
            break;
        case SE_HW_PARAM_1:
            r = qupv3_handle[SE_HW_PARAM_1];
            SCP_DEBUG(()) << hex << "Unhandled READ at offset :" << offset;
            break;
        case UART_MANUAL_RFR:
            r = qupv3_handle[UART_MANUAL_RFR];
            SCP_DEBUG(())("Manual RFR read");
            break;
        default:
            SCP_WARN(()) << "Error: qupv3_read() Unhandled read(" << hex << offset
                         << ") :  " << SE_HW_PARAM_0;
            break;
            r = 0;
            break;
        }

        return r;
    }

    void qupv3_write(uint64_t offset, uint32_t value) {
        unsigned char ch;
        SCP_DEBUG(()) << "in qupv3_write()";
        switch (offset) {
        case GENI_M_CMD_0:
            /* Handle start of the UART Tx transection */
            SCP_DEBUG(()) << hex << "Addr(GENI_M_CMD_0):" << value;
            if ((qupv3_handle[GENI_M_CMD_0] == 0x0) && (value == 0x08000000)) {
                qupv3_handle[GENI_M_CMD_0] = value;
            }
            break;
        case GENI_M_IRQ_CLEAR:
            /* Clear interrupt before starting UART Tx transection */
            SCP_DEBUG(()) << hex << "Addr(GENI_M_IRQ_CLEAR):" << value;
            qupv3_handle[GENI_M_IRQ_STATUS] &= ~(value);
            qupv3_update();
            break;
        case GENI_S_IRQ_CLEAR:
            /* Clear interrupt before starting UART Tx transection */
            SCP_DEBUG(()) << hex << "Addr(GENI_S_IRQ_CLEAR):" << value;
            qupv3_handle[GENI_S_IRQ_STATUS] &= ~(value);
            qupv3_update();
            break;
        case GENI_TX_FIFO_0:
            /* Put single byte to this FIFO register and it should needs to reflect on UART Tx
             * line
             */
            ch = value;
            if ((qupv3_handle[GENI_M_CMD_0] == 0x08000000) &&
                (qupv3_handle[UART_TX_TRANS_LEN] >= 0x1)) {
                int count = 0;

                /* Write up to 4 bytes from single FIFO on every request from software */
                while ((count != 4) && (qupv3_handle[UART_TX_TRANS_LEN] != 0)) {
                    ch = ((value >> (count * 8)) & 0xff);
                    chr->write(ch);
                    count++;
                    qupv3_handle[UART_TX_TRANS_LEN] -= 1;
                }

                /* If no more data is left to write then set CMD_DONE irq bit */
                if ((qupv3_handle[GENI_M_CMD_0] == 0x08000000) &&
                    (qupv3_handle[UART_TX_TRANS_LEN] == 0)) {
                    qupv3_handle[GENI_M_CMD_0] = 0x0;
                    qupv3_handle[GENI_M_IRQ_STATUS] |= M_CMD_DONE;
                    qupv3_update();
                }
            } else {
                SCP_ERR() << "Error: M_CMD_0 and UART_TX_LEN is not set properly";
            }
            SCP_DEBUG(()) << "Char to Tx Addr(GENI_TX_FIFO_0): \"" << ch << "\" ";
            break;
        case UART_TX_TRANS_LEN:
            /* Number of bytes to transfer in single transection, for now it should be one byte
             */
            SCP_DEBUG(()) << hex << "Addr(UART_TX_TRANS_LEN):" << value;
            if (value >= 0x1)
                qupv3_handle[UART_TX_TRANS_LEN] = value;
            else
                SCP_ERR() << hex << "Error: Addr(UART_TX_TRANS_LEN):" << value;
            break;
        case UART_MANUAL_RFR:
            qupv3_handle[UART_MANUAL_RFR] = value;
            SCP_DEBUG(())("Manual RFR write");
            break;

        case SE_GSI_EVENT_EN:
        case GENI_S_IRQ_ENABLE:
        case SE_IRQ_EN:
        case GENI_M_IRQ_EN_CLEAR:
        case GENI_S_IRQ_EN_CLEAR:
        case GENI_M_IRQ_EN_SET:
        case GENI_S_IRQ_EN_SET:
        case GENI_DMA_MODE_EN:
        case GENI_S_CMD0:
        case UNKNOWN_TX_FIFO:
        case GENI_RX_WATERMARK_REG:
        case GENI_RX_RFR_WATERMARK_REG:
            SCP_DEBUG(()) << hex << "Unhandled WRITE at offset :" << offset;
            break;
        default:
            SCP_WARN(()) << hex << "Error: qupv3_write() Unhandled write(" << offset
                         << "): " << value;
        }
    }
    int get_fifo_has_space() {
        return ((qupv3_handle[GENI_RX_FIFO_STATUS] >> 28) < (GENI_RX_FIFO_MAX - 1)) &&
               (qupv3_handle[UART_MANUAL_RFR] != 0x80000002);
    }
    static int qupv3_can_receive(void* opaque) {
        QUPv3* uart = (QUPv3*)opaque;
        return uart->get_fifo_has_space();
    }

    void qupv3_put_fifo(uint32_t value) {
        int n = (qupv3_handle[GENI_RX_FIFO_STATUS] >> 28) & (GENI_RX_FIFO_MAX - 1);
        assert(n < GENI_RX_FIFO_MAX);
        qupv3_handle[GENI_RX_FIFO_0 + n] = value;
        n++;
        qupv3_handle[GENI_S_IRQ_STATUS] |= (RX_FIFO_LAST);
        qupv3_handle[GENI_RX_FIFO_STATUS] = (RX_LAST) | (n << 28);
        qupv3_update();
    }

    static void qupv3_receive(void* opaque, const uint8_t* buf, int size) {
        QUPv3* uart = (QUPv3*)opaque;
        uart->qupv3_put_fifo(*buf);
    }
};
#endif

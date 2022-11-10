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

#ifndef __QUPv3_SSPI__
#define __QUPv3_SSPI__

#include <mutex>
#include <queue>

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include <systemc>

#include <greensocs/gsutils/ports/initiator-signal-socket.h>
#include <greensocs/gsutils/ports/target-signal-socket.h>
#include <greensocs/libgssync/async_event.h>

#include <unordered_map>
#include <scp/report.h>

#include "qupv3_regs.h"

void qupv3_write(uint64_t offset, uint32_t value);
uint32_t qupv3_read(uint64_t offset);

typedef struct SPIState_S {
    int read_pos;      //Current read_fifo slot position
    int read_slots;    //Total slots to be read
    int read_count;    //Number of current bytes received from slave
    int read_start;    //Rx from Slave started
    int read_done;     //Rx from Slave completed
    int read_len;      //storing Master write length
    int read_pending;  //status of read
    int write_pos;     //current write_fifo slot position
    int write_slots;   //Total slots to be written
    int write_count;   //Number of current bytes sent to slave
    int write_start;   //Tx to slave started
    int write_done;    //Tx to slave completed
    int write_len;     //storing slave write length
    int write_pending; //status of write
    int opcode;        //store current opcode of transaction
    int loopback;      //loopback information
} SPIState_S;

class QUPv3_SpiS : public sc_core::sc_module
{
public:
    std::unique_ptr<SPIState_S> s;

    /* Initiator and Target Sockets */
    tlm_utils::simple_target_socket<QUPv3_SpiS> socket;
    tlm_utils::simple_target_socket<QUPv3_SpiS> s_socket;

    InitiatorSignalSocket<bool> irq;
    TargetSignalSocket<bool> dummy_target; //only for testing purpose

    /* Queues for storing data */
    std::queue<uint8_t> read_queue;
    std::queue<uint8_t> write_queue;

    /* QUPv3 : <address, data> Handle entity to used in Software */
    std::unordered_map<uint32_t, uint32_t> qupv3_handle = {
        { GENI_FORCE_DEFAULT_REG, 0x0 },
        { GENI_OUTPUT_CTRL, 0x0 },
        { GENI_CGC_CTRL, 0x78 },
        { GENI_SER_M_CLK_CFG, 0x11 },
        { GENI_FW_REVISION, 0x500 }, //FW version as per HALqupe.h
        { GENI_CLK_SEL, 0x0 },
        { GENI_DFS_IF_CFG, 0x300 },
        { GENI_CFG_SEQ_START, 0x0 },
        { GENI_CFG_STATUS, 0x0 },

        { SPI_CPHA, 0x0 },
        { SPI_LOOPBACK_CFG, 0x0 },
        { SPI_CPOL, 0x0 },
        { GENI_CFG_REG80, 0x0 },
        { SPI_DEMUX_OUTPUT_INV, 0x0 },
        { SPI_DEMUX_SEL, 0x0 },
        { GENI_DMA_MODE_EN, 0x0 },
        { GENI_TX_PACKING_CFG0, 0x7F8FE },
        { GENI_TX_PACKING_CFG1, 0x7FEFE },
        { SPI_WORD_LEN, 0x0 },
        { SPI_TX_TRANS_LEN, 0x0 },
        { SPI_RX_TRANS_LEN, 0x0 },
        { SPI_PRE_POST_CMD_DLY, 0x0 },
        { SPI_DELAYS_COUNTERS, 0x400000 },
        { GENI_RX_PACKING_CFG0, 0x7F8FE },
        { GENI_RX_PACKING_CFG1, 0x7FEFE },
        { SE_SPI_SLAVE_EN, 0x0 },

        { GENI_M_CMD_0, 0x0 },
        { GENI_M_CMD_CTRL_REG, 0x0 },
        { GENI_M_IRQ_STATUS, 0x0 },
        { GENI_M_IRQ_ENABLE, 0x0 },
        { GENI_M_IRQ_CLEAR, 0x0 },
        { GENI_S_IRQ_STATUS, 0x0 },
        { GENI_S_IRQ_ENABLE, 0x0 },
        { GENI_S_IRQ_CLEAR, 0x0 },
        { GENI_TX_FIFO_0, 0x0 },
        { GENI_RX_FIFO_0, 0x0 },
        { GENI_TX_FIFO_STATUS, 0x0 },
        { GENI_RX_FIFO_STATUS, 0x0 },
        { GENI_TX_WATERMARK_REG, 0x0 },
        { GENI_RX_WATERMARK_REG, 0x0 },
        { GENI_RX_RFR_WATERMARK_REG, 0xE },

        { DMA_TX_IRQ_CLR, 0x0 },
        { DMA_RX_IRQ_CLR, 0x0 },
        { SE_GSI_EVENT_EN, 0x0 },
        { SE_IRQ_EN, 0x0 }, //Forcing reset values of DMA IRQ to zero
        { HW_PARAM_0, 0x20102BE4 },
        { HW_PARAM_1, 0x20104800 },
        { DMA_GENERAL_CFG, 0x0 }, //Forcing contents to zero since DMA mode is not enabled
    };

    SC_HAS_PROCESS(QUPv3_SpiS);
    QUPv3_SpiS(sc_core::sc_module_name name)
        : irq("irq")
        , s_socket("s_socket")
        , dummy_target("dummy_target")
        , s(std::make_unique<SPIState_S>()) {
        /* Register b_transports */
        socket.register_b_transport(this, &QUPv3_SpiS::b_transport);
        s_socket.register_b_transport(this, &QUPv3_SpiS::m2s_xchange);
    }

    /* b_transport function for socket */
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
        unsigned char* ptr = trans.get_data_ptr();
        uint64_t addr      = trans.get_address();
        uint32_t ptr_data  = 0x0;

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

    /* b_transport function for s_socket */
    void m2s_xchange(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) {
        unsigned char* ptr = trans.get_data_ptr();
        uint64_t addr      = trans.get_address();
        uint32_t ptr_data  = 0x0;

        trans.set_dmi_allowed(false);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);

        switch (trans.get_command()) {
        case tlm::TLM_WRITE_COMMAND:
            memcpy(&ptr_data, ptr, sizeof(uint32_t));
            master_write(addr, ptr_data);
            break;
        case tlm::TLM_READ_COMMAND:
            ptr_data = master_read(addr);
            memcpy(ptr, &ptr_data, sizeof(uint32_t));
            break;
        default:
            break;
        }
    }

    // Print the queue
    void showq(std::queue<uint8_t> gq) {
        std::queue<uint8_t> g = gq;
        uint8_t val;
        while (!g.empty()) {
            val = g.front();
            std::cout << val << std::endl;
            g.pop();
        }
        std::cout << '\n';
    }

    void print_reg() {
        for (auto i = qupv3_handle.begin(); i != qupv3_handle.end(); i++) {
            std::cout << i->first << " : " << i->second << '\n';
        }
    }

    void trigger_irq() { //sending positive edge pulse
        SCP_DEBUG() << "SLAVE-Triggering IRQ";
        irq->write(0);
        irq->write(1);
    }

    /* TLM write handler for transactions from s_socket */
    void master_write(uint64_t offset, uint32_t value) {
        SCP_DEBUG() << std::hex << "SLAVE -Input from master - offset: " << offset << " and value:" << value;
        uint16_t rx_last_bytes_valid = 0x0;
        uint16_t slots               = 0x0;
        uint8_t num_bytes            = 0x0;
        switch (offset) {
        case SPI_RX_TRANS_LEN:
            //qupv3_handle[SPI_RX_TRANS_LEN] = value;                   //Update length as per master transaction
            s->read_len = value; //Store value in structure
            SCP_DEBUG() << std::hex << "SLAVE- Rx length Transaction from Master: " << value;

            s->read_pos   = 0x0; //Initialize postion to zero
            s->read_count = 0x0; //Initializing current count to zero
            break;

        case GENI_RX_FIFO_0:

            //Calculate current read_count
            if ((s->read_len == FIFO_ENTRY_SIZE) || ((s->read_len - s->read_count) >= FIFO_ENTRY_SIZE)) {
                s->read_count = s->read_count + FIFO_ENTRY_SIZE;
                num_bytes     = FIFO_ENTRY_SIZE;
            } else {
                s->read_count = s->read_count + (s->read_len % FIFO_ENTRY_SIZE);
                num_bytes     = (s->read_len % FIFO_ENTRY_SIZE);
            }

            s->read_pos++;

            while (num_bytes != 0) { //Update value to FIFO
                read_queue.push(value & 0xff);
                value = (value >> 8);
                num_bytes--;
            }

            slots = (s->read_len / FIFO_ENTRY_SIZE) + ((s->read_len % FIFO_ENTRY_SIZE != 0) ? 1 : 0); //Number of slots to be read. Each slot = 4bytes

            if (s->read_pos == slots) {
                s->read_pos   = 0x0; //Initialize postion to zero
                s->read_count = 0x0; //clearing read_count

                /*                  SCP_DEBUG() << "SLAVE Rx FIFO Data(M->S): ");                //Print M->S contents
                    showq(read_queue);
*/
                if (s->read_start == 0x1) { //pending read stalled
                    if (read_queue.size() >= qupv3_handle[SPI_RX_TRANS_LEN]) {
                        qupv3_handle[GENI_M_IRQ_STATUS] |= (RX_FIFO_WATERMARK);
                        trigger_irq();
                    }
                }
            }
            break;

        default:
            SCP_ERR() << "SLAVE- Unhandled transaction from Master";
        }
    }

    /* TLM Read handler for transactions from s_socket */
    uint32_t master_read(uint64_t offset) {
        uint32_t r         = 0x0;
        uint32_t num_bytes = 0x0;
        uint32_t value, i = 0x0;

        switch (offset) {
        case SPI_TX_TRANS_LEN:
            r = s->write_len; //Send saved Tx_trans_len
            SCP_DEBUG() << std::hex << "SLAVE-Tx length Transaction from Master: " << r;

            s->write_count = 0x0; //Initializing write_count to zero
            break;
        case GENI_TX_FIFO_0:
            //Calculate current read_count
            if ((s->write_len == FIFO_ENTRY_SIZE) || ((s->write_len - s->write_count) >= FIFO_ENTRY_SIZE)) {
                s->write_count = s->write_count + FIFO_ENTRY_SIZE;
                num_bytes      = FIFO_ENTRY_SIZE;
            } else {
                s->write_count = s->write_count + (s->write_len % FIFO_ENTRY_SIZE);
                num_bytes      = (s->write_len % FIFO_ENTRY_SIZE);
            }

            value = 0x0;
            i     = 0x0;
            while (num_bytes != 0) {
                SCP_DEBUG() << std::hex << "SLAVE-Queue front: " << (int)write_queue.front();
                value = (value | (write_queue.front() << (i * 8)));
                write_queue.pop();
                num_bytes--;
                i++;
            }
            r = value; //Read content from write fifo

            break;
        default:
            SCP_ERR() << "SLAVE- Unhandled transaction from Master";
        }
        return r;
    }

    /* TLM Read handler for transactions from socket */
    uint32_t qupv3_read(uint64_t offset) {
        uint32_t r;
        uint32_t words_to_read       = 0x0;
        uint32_t value               = 0x0;
        uint8_t i                    = 0x0;
        uint32_t rx_last_bytes_valid = 0x0;
        uint8_t num_bytes            = 0;

        switch (offset) {
        case GENI_OUTPUT_CTRL:
        case GENI_CGC_CTRL:
        case GENI_SER_M_CLK_CFG:
        case GENI_CLK_SEL:
        case GENI_DFS_IF_CFG:
        case SPI_CPHA:
        case SPI_LOOPBACK_CFG:
        case SPI_CPOL:
        case GENI_FW_REVISION:
        case GENI_CFG_REG80:
        case SPI_DEMUX_OUTPUT_INV:
        case GENI_DMA_MODE_EN:
        case GENI_TX_PACKING_CFG0:
        case GENI_TX_PACKING_CFG1:
        case GENI_RX_PACKING_CFG0:
        case GENI_RX_PACKING_CFG1:
        case SPI_WORD_LEN:
        case SPI_TX_TRANS_LEN:
        case SPI_RX_TRANS_LEN:
        case SPI_DEMUX_SEL:
        case SPI_PRE_POST_CMD_DLY:
        case SPI_DELAYS_COUNTERS:
        case SE_SPI_SLAVE_EN:
        case GENI_CFG_STATUS:
        case GENI_M_CMD_0:
        case GENI_M_CMD_CTRL_REG:
            /* Read registers based on offset */
            r = qupv3_handle[offset];
            SCP_DEBUG() << std::hex << "SLAVE READ - Addr and value:" << offset << "  " << r;
            break;

        case GENI_M_IRQ_STATUS:
            /* Read M_IRQ_STATUS register */
            r = qupv3_handle[GENI_M_IRQ_STATUS];
            SCP_DEBUG() << std::hex << "SLAVE READ - Addr and value:" << offset << "  " << r;
            break;
        case GENI_M_IRQ_ENABLE:
            /* Read M_IRQ_ENABLE register */
            if (qupv3_handle[SE_IRQ_EN] & GEN_M_EN)
                r = qupv3_handle[GENI_M_IRQ_ENABLE];
            else {
                r                               = 0;
                qupv3_handle[GENI_M_IRQ_ENABLE] = 0;
            }
            SCP_DEBUG() << std::hex << "SLAVE READ - Addr and value:" << offset << "  " << r;

            break;
        case GENI_S_IRQ_STATUS:
            /* Read S_IRQ_STATUS register */
            r = qupv3_handle[GENI_S_IRQ_STATUS];
            SCP_DEBUG() << std::hex << "SLAVE READ - Addr and value:" << offset << "  " << r;
            qupv3_handle[GENI_S_IRQ_STATUS] = 0x0;
            break;
        case GENI_S_IRQ_ENABLE:
            /* Read S_IRQ_ENABLE register */
            if (qupv3_handle[SE_IRQ_EN] & GEN_S_EN)
                r = qupv3_handle[GENI_S_IRQ_ENABLE];
            else {
                r                               = 0;
                qupv3_handle[GENI_S_IRQ_ENABLE] = 0;
            }
            break;
        case GENI_RX_FIFO_0:
            /* Return Rx FIFO data which we have read from Master */
            if (s->read_pos == 0x0) {
                SCP_INFO() << std::hex << "****SLAVE-RX Started with RX_TRANS_LEN: " << qupv3_handle[SPI_RX_TRANS_LEN] << " ****";
                s->read_slots                     = (qupv3_handle[SPI_RX_TRANS_LEN] / FIFO_ENTRY_SIZE) + ((qupv3_handle[SPI_RX_TRANS_LEN] % FIFO_ENTRY_SIZE != 0) ? 1 : 0); //Storing FIFO slots in structure. Each slot = 4 bytes
                qupv3_handle[GENI_RX_FIFO_STATUS] = s->read_slots;                                                                                                          //Initialize with total word count
            }

            //Read current word count needed to be read by software
            words_to_read = qupv3_handle[GENI_RX_FIFO_STATUS] & 0x1ffffff;
            words_to_read = words_to_read - 1;

            //Calculate current read_count
            if ((qupv3_handle[SPI_RX_TRANS_LEN] == FIFO_ENTRY_SIZE) || ((qupv3_handle[SPI_RX_TRANS_LEN] - s->read_count) >= FIFO_ENTRY_SIZE)) {
                s->read_count = s->read_count + FIFO_ENTRY_SIZE;
                num_bytes     = FIFO_ENTRY_SIZE;
            } else {
                s->read_count = s->read_count + (qupv3_handle[SPI_RX_TRANS_LEN] % FIFO_ENTRY_SIZE);
                num_bytes     = (qupv3_handle[SPI_RX_TRANS_LEN] % FIFO_ENTRY_SIZE);
            }

            value = 0x0;
            i     = 0;
            while (num_bytes != 0) {
                value = (value | (read_queue.front() << (i * 8)));
                read_queue.pop();
                num_bytes--;
                i++;
            }

            //Get data from read_fifo which was written by Master
            s->read_pos++;
            qupv3_handle[GENI_RX_FIFO_0] = value;
            r                            = qupv3_handle[GENI_RX_FIFO_0];
            SCP_INFO() << std::hex << "SLAVE READ - Addr and value:" << offset << "  " << r;

            if (words_to_read == 0x1) {
                //Calculate valid bytes in final transaction
                if (qupv3_handle[SPI_RX_TRANS_LEN] == FIFO_ENTRY_SIZE) {
                    rx_last_bytes_valid = FIFO_ENTRY_SIZE;
                } else if ((qupv3_handle[SPI_RX_TRANS_LEN] - s->read_count) == FIFO_ENTRY_SIZE) {
                    rx_last_bytes_valid = FIFO_ENTRY_SIZE;
                } else if ((qupv3_handle[SPI_RX_TRANS_LEN] - s->read_count) < FIFO_ENTRY_SIZE) {
                    rx_last_bytes_valid = (qupv3_handle[SPI_RX_TRANS_LEN] - s->read_count);
                } else {
                    rx_last_bytes_valid = 0x0;
                }

                //Update Rx_LAST and valid bytes to be read in final transaction
                qupv3_handle[GENI_RX_FIFO_STATUS] = ((qupv3_handle[GENI_RX_FIFO_STATUS] & 0x0fffffff) | (rx_last_bytes_valid << RX_LAST_BYTE_VALID_OFF)) | RX_LAST; //Modifying last bytes valid
            }

            if (s->read_count == qupv3_handle[SPI_RX_TRANS_LEN]) {
                //Setting RX_FIFO_LAST in status register
                if (qupv3_handle[GENI_M_IRQ_ENABLE] & RX_FIFO_LAST) {
                    qupv3_handle[GENI_M_IRQ_STATUS] |= RX_FIFO_LAST;
                }

                SCP_INFO() << "****SLAVE RX Ended****";
                //print_reg();
                s->read_done                      = 0x1; //setting done to one since Rx completed
                s->read_start                     = 0x0; //clearing start
                s->read_pos                       = 0x0; //clearing read_pos
                s->write_done                     = 0x0; //clearing write_done
                qupv3_handle[GENI_M_CMD_0]        = 0x0; //clearing for next transaction
                qupv3_handle[GENI_RX_FIFO_STATUS] = 0x0; //Clearing FIFO status

                //update CMD_DONE and trigger_irq
                if ((qupv3_handle[GENI_M_IRQ_ENABLE] & M_CMD_DONE) && (qupv3_handle[SPI_RX_TRANS_LEN] != 0)) {
                    qupv3_handle[GENI_M_IRQ_STATUS] |= (M_CMD_DONE);
                    trigger_irq();
                }
                /*    
                SCP_DEBUG()<< "SLAVE Rx FIFO Data(S->M): ");                         //Print Rx Fifo data contents
                showq(read_queue);
*/
            } else {
                //Updating wordcount needed to be read by software
                qupv3_handle[GENI_RX_FIFO_STATUS] = ((qupv3_handle[GENI_RX_FIFO_STATUS] & 0xfe000000) | (words_to_read)); //Modifying Wordcount
            }

            break;
        case GENI_TX_FIFO_STATUS:
            /* Read GENI_TX_FIFO_STATUS register */
            r = qupv3_handle[GENI_TX_FIFO_STATUS];
            SCP_DEBUG() << std::hex << "SLAVE READ - Addr and value:" << offset << "  " << r;
            break;

        case GENI_RX_FIFO_STATUS:
            /* Read GENI_RX_FIFO_STATUS register */
            rx_last_bytes_valid = 0x0;
            //Initialize FIFO status for read transaction
            if ((s->read_pos == 0x0) && (s->read_start == 0x1)) {
                s->read_slots                     = (qupv3_handle[SPI_RX_TRANS_LEN] / FIFO_ENTRY_SIZE) + ((qupv3_handle[SPI_RX_TRANS_LEN] % FIFO_ENTRY_SIZE != 0) ? 1 : 0); //Storing FIFO slots in structure. Each slot = 4 bytes
                qupv3_handle[GENI_RX_FIFO_STATUS] = s->read_slots;                                                                                                          //Initialize with total word count

                //Calculate valid bytes in final transaction for wordcount=1 or total length <=4
                if (s->read_slots == 0x1) {
                    rx_last_bytes_valid               = qupv3_handle[SPI_RX_TRANS_LEN];
                    qupv3_handle[GENI_RX_FIFO_STATUS] = ((qupv3_handle[GENI_RX_FIFO_STATUS] & 0x0fffffff) | (rx_last_bytes_valid << RX_LAST_BYTE_VALID_OFF)) | RX_LAST; //Modifying last bytes valid
                }
            }

            r = qupv3_handle[GENI_RX_FIFO_STATUS];
            if ((r & 0x1ffffff) == 0x0) { //check Wordcount=0. All words are read and hence clearing status
                r                                 = 0x0;
                qupv3_handle[GENI_RX_FIFO_STATUS] = 0x0;
            }

            SCP_DEBUG() << std::hex << "SLAVE READ - Addr and value:" << offset << "  " << r;
            break;

        case GENI_TX_WATERMARK_REG:
        case GENI_RX_WATERMARK_REG:
        case GENI_RX_RFR_WATERMARK_REG:
        case SE_GSI_EVENT_EN:
        case SE_IRQ_EN:
        case HW_PARAM_0:
        case HW_PARAM_1:
        case DMA_GENERAL_CFG:
            /* Read registers based on offset */
            r = qupv3_handle[offset];
            SCP_DEBUG() << std::hex << "SLAVE READ - Addr and value:" << offset << "  " << r;
            break;

        default:
            SCP_ERR() << "Slave: Unhandled read(" << std::hex << offset << ") ";
            r = 0;
        }

        return r;
    }

    /* TLM write handler for transactions from socket */
    void qupv3_write(uint64_t offset, uint32_t value) {
        unsigned char ch;
        int num_bytes = 0x0;
        int nbytes    = 0x0;
        int val       = 0x0;
        switch (offset) {
        case GENI_FORCE_DEFAULT_REG:
            //TODO: force all GENI register to default values if value is one
            qupv3_handle[GENI_FORCE_DEFAULT_REG] = value;
            SCP_DEBUG() << std::hex << "SLAVE WRITE - Addr and value:" << offset << "  " << value;

            break;

        case GENI_CFG_SEQ_START:
        case GENI_OUTPUT_CTRL:
        case GENI_CFG_REG80:
        case SE_SPI_SLAVE_EN:
            /* Write registers based on offset */
            qupv3_handle[offset] = value;
            SCP_DEBUG() << std::hex << "SLAVE WRITE - Addr and value:" << offset << "  " << value;

            break;

        case GENI_CGC_CTRL:
        case GENI_SER_M_CLK_CFG:
        case GENI_CLK_SEL:
        case GENI_DFS_IF_CFG:

        case SPI_CPHA:
        case SPI_CPOL:
        case SPI_DEMUX_OUTPUT_INV:
            //Based on cs_demux_output_inv, output polarity should be changed
        case SPI_DEMUX_SEL: //SlaveID. As of now, only one slave is present
        case GENI_TX_PACKING_CFG0:
        case GENI_TX_PACKING_CFG1:
        case SPI_WORD_LEN:
        case SPI_PRE_POST_CMD_DLY:
        case SPI_DELAYS_COUNTERS:
        case GENI_RX_PACKING_CFG0:
        case GENI_RX_PACKING_CFG1:
            /* Write registers based on offset */
            SCP_DEBUG() << std::hex << "SLAVE WRITE - Addr and value:" << offset << "  " << value;
            qupv3_handle[offset] = value;
            break;

        case SPI_LOOPBACK_CFG:
            /*Write to loopback config register */
            SCP_INFO() << std::hex << "SLAVE WRITE - Addr and value:" << offset << "  " << value;
            qupv3_handle[SPI_LOOPBACK_CFG] = value;
            if (value == 0x1)
                s->loopback = 0x1;
            break;

        case GENI_DMA_MODE_EN:
            /* Write to GENI_DMA_MODE_EN register */
            //Only FIFO mode supported.
            qupv3_handle[GENI_DMA_MODE_EN] = 0;
            SCP_DEBUG() << std::hex << "SLAVE WRITE - Addr and value:" << offset << "  " << value;

            break;

        case SPI_TX_TRANS_LEN:
            /* Number of bytes to transfer in write transaction */
            SCP_INFO() << std::hex << "SLAVE - Addr(SPI_TX_TRANS_LEN) and value:" << offset << "  " << value;
            if (value >= 0x1) {
                qupv3_handle[SPI_TX_TRANS_LEN]    = value;
                s->write_slots                    = (value / FIFO_ENTRY_SIZE) + ((value % FIFO_ENTRY_SIZE != 0) ? 1 : 0); //Storing FIFO slots in structure. Each slot = 4 bytes
                qupv3_handle[GENI_TX_FIFO_STATUS] = s->write_slots;                                                       //Initialize with total word count
                s->write_len                      = value;                                                                //Storing total length in structure

                s->write_pos   = 0x0; //Initializing starting index number for writing data into FIFO
                s->write_start = 0x0; //Setting start to zero
                s->write_done  = 0x0; //Setting done to zero
                s->write_count = 0x0; //Initialize current byte count to zero
            } else {
                qupv3_handle[SPI_TX_TRANS_LEN] = 0x0;
                SCP_ERR() << std::hex << "Addr(SPI_TX_TRANS_LEN):" << value;
            }
            break;
        case SPI_RX_TRANS_LEN:
            /* Number of bytes to transfer in read transaction */
            SCP_INFO() << std::hex << "SLAVE WRITE - Addr(SPI_RX_TRANS_LEN) and value:" << offset << "  " << value;
            if (value >= 0x1) {
                qupv3_handle[SPI_RX_TRANS_LEN] = value;

                s->read_pos   = 0x0; //Initializing starting index number for reading data from FIFO
                s->read_start = 0x0; //Setting start to zero
                s->read_done  = 0x0; //Setting done to zero
                s->read_count = 0x0; //Initialize current byte count to zero
            } else {
                qupv3_handle[SPI_RX_TRANS_LEN] = 0x0;
                SCP_ERR() << std::hex << "Addr(SPI_RX_TRANS_LEN):" << value;
            }
            break;
        case GENI_M_CMD_0:
            /* Handle Opcodes of the SPI transaction */
            SCP_INFO() << std::hex << "SLAVE Addr(GENI_M_CMD_0):" << value;
            s->opcode = value >> M_CMD_OPCODE_START; //Extract incoming opcode

            if (qupv3_handle[GENI_M_CMD_0] != 0x0) {
                SCP_DEBUG() << "SLAVE Active transfer. Trying to modify M_CMD0 register. Making register value as per new value";
                qupv3_handle[GENI_M_CMD_0] = 0x0;
            }

            if ((qupv3_handle[GENI_M_CMD_0] == 0x0)) { //Feed the value only if register is cleared from previous transaction

                //clear IRQ status before starting transactions
                qupv3_handle[GENI_M_IRQ_STATUS] = 0x0;
                qupv3_handle[GENI_S_IRQ_STATUS] = 0x0;

                //opcode handling
                switch (s->opcode) {
                case SPI_TX_ONLY_OPCODE:
                    qupv3_handle[GENI_M_CMD_0] = value;

                    //Update Tx watermark IRQ and trigger IRQ
                    qupv3_handle[GENI_M_IRQ_STATUS] = (qupv3_handle[GENI_M_IRQ_STATUS] | TX_FIFO_WATERMARK);
                    trigger_irq();
                    break;

                case SPI_RX_ONLY_OPCODE:
                    qupv3_handle[GENI_M_CMD_0] = value;

                    //Update Rx watermark IRQ and trigger IRQ
                    s->read_start = 0x1;
                    if (read_queue.size() >= qupv3_handle[SPI_RX_TRANS_LEN]) { //Data available, trigger interrupt
                        qupv3_handle[GENI_M_IRQ_STATUS] |= (RX_FIFO_WATERMARK);
                        trigger_irq();
                    }
                    break;

                case SPI_FULL_DUPLEX_OPCODE:
                case SPI_TX_RX_OPCODE:
                    if (qupv3_handle[SPI_TX_TRANS_LEN] == 0x0) {
                        SCP_INFO() << "SLAVE SPI_TX_TRANS_LEN=0x0. Hence skipping Tx";
                        //Update Rx watermark IRQ and trigger IRQ
                        s->read_start = 0x1;
                        if (read_queue.size() >= qupv3_handle[SPI_RX_TRANS_LEN]) { //Data available, trigger interrupt
                            qupv3_handle[GENI_M_IRQ_STATUS] |= (RX_FIFO_WATERMARK);
                            trigger_irq();
                        }
                    } else {
                        //Update Tx watermark IRQ and trigger IRQ
                        qupv3_handle[GENI_M_IRQ_STATUS] = (qupv3_handle[GENI_M_IRQ_STATUS] | TX_FIFO_WATERMARK);
                        trigger_irq();
                    }
                    qupv3_handle[GENI_M_CMD_0] = value;
                    break;

                default:
                    SCP_ERR() << std::hex << "SLAVE M_CMD0 - Trying to write unsupported SPI Mode:" << s->opcode;
                }
            }
            break;
        case GENI_M_CMD_CTRL_REG:
            /* Write to GENI_M_CMD_CTRL_REG register */
            SCP_INFO() << std::hex << "SLAVE WRITE - Addr and value:" << offset << "  " << value;

            //handle abort and cancel transmission bits
            qupv3_handle[GENI_M_CMD_CTRL_REG] = value;
            if (qupv3_handle[GENI_M_CMD_CTRL_REG] & M_CMD_CTRL_ABORT) {
                qupv3_handle[GENI_M_IRQ_STATUS] |= M_CMD_ABORT; //Abort sequence steps
            } else if (qupv3_handle[GENI_M_CMD_CTRL_REG] & M_CMD_CTRL_CANCEL) {
                qupv3_handle[GENI_M_IRQ_STATUS] |= M_CMD_CANCEL; //Cancel sequence steps
            }

            qupv3_handle[GENI_M_CMD_CTRL_REG] = 0x0; //Request is handled
            trigger_irq();
            break;

        case GENI_M_IRQ_ENABLE:
            //Setting IRQ enable
            SCP_DEBUG() << std::hex << "SLAVE WRITE - Addr and value:" << offset << "  " << value;
            if (qupv3_handle[SE_IRQ_EN] & GEN_M_EN)
                qupv3_handle[offset] = value;
            break;
        case GENI_S_IRQ_ENABLE:
            //Setting IRQ enable. SPI used M_IRQ only for transaction
            SCP_DEBUG() << std::hex << "SLAVE WRITE - Addr and value:" << offset << "  " << value;
            if (qupv3_handle[SE_IRQ_EN] & GEN_S_EN)
                qupv3_handle[offset] = value;
            break;
        case GENI_M_IRQ_CLEAR:
            /* Clear interrupt before starting transaction */
            SCP_DEBUG() << std::hex << "SLAVE WRITE - Addr and value:" << offset << "  " << value;
            qupv3_handle[GENI_M_IRQ_STATUS] &= ~(value);
            irq->write(0); //Make interrupt line low

            if (value & TX_FIFO_WATERMARK) { //Once TX_FIFO_Watermark is cleared from TX, start Rx process
                if ((s->write_done == 0x1) && ((s->opcode == SPI_FULL_DUPLEX_OPCODE) || (s->opcode == SPI_TX_RX_OPCODE))) {
                    //Update Rx watermark IRQ and trigger IRQ
                    s->read_start = 0x1;
                    if (read_queue.size() >= qupv3_handle[SPI_RX_TRANS_LEN]) { //Data available, trigger interrupt
                        qupv3_handle[GENI_M_IRQ_STATUS] |= (RX_FIFO_WATERMARK);
                        trigger_irq();
                    }
                }
            }
            break;
        case GENI_S_IRQ_CLEAR:
            /* Clear interrupt before starting transaction */
            SCP_DEBUG() << std::hex << "SLAVE WRITE - Addr and value:" << offset << "  " << value;
            qupv3_handle[GENI_S_IRQ_STATUS] &= ~(value);
            irq->write(0); //Make interrupt line low
            break;
        case GENI_TX_FIFO_0:
            /* Put single byte to this FIFO register and it should needs to reflect on Tx line */
            SCP_INFO() << std::hex << "SLAVE WRITE - Addr and value:" << offset << "  " << value;
            ch                           = value;
            qupv3_handle[GENI_TX_FIFO_0] = value;

            if ((qupv3_handle[SPI_LOOPBACK_CFG] == 0x1) && (s->opcode != SPI_TX_ONLY_OPCODE)) //supported only in full duplex modes
                s->loopback = 0x1;
            else
                s->loopback = 0x0;

            if (qupv3_handle[SPI_TX_TRANS_LEN] >= 0x1) {
                if ((s->opcode == SPI_TX_ONLY_OPCODE) || (s->opcode == SPI_FULL_DUPLEX_OPCODE) || (s->opcode = SPI_TX_RX_OPCODE)) { //Opcodes supported for Tx
                    if (s->write_start == 0x0) {
                        s->write_done  = 0x0;
                        s->write_start = 0x1;
                        SCP_INFO() << std::hex << "****SLAVE TX Started with TX_TRANS_LEN: " << qupv3_handle[SPI_TX_TRANS_LEN] << " ****";
                        SCP_DEBUG() << std::hex << "SLAVE Loopback:" << s->loopback;
                    }

                    int count = 0;
                    int data  = 0;

                    //Check for extracting data from last 4bytes as per Tx_len
                    if ((qupv3_handle[SPI_TX_TRANS_LEN] / FIFO_ENTRY_SIZE) > 0)
                        data = value;
                    else
                        data = value & ((1 << (qupv3_handle[SPI_TX_TRANS_LEN] * 8)) - 1); //1 byte - 0xff, 2bytes - 0xffff, 3 bytes - 0xffffff

                    //Track current write bytes received
                    if ((qupv3_handle[SPI_TX_TRANS_LEN] / FIFO_ENTRY_SIZE) > 0) {
                        s->write_count = s->write_count + FIFO_ENTRY_SIZE; //Max 4bytes can be FIFO at a time
                        num_bytes      = FIFO_ENTRY_SIZE;
                    } else {
                        s->write_count = s->write_count + qupv3_handle[SPI_TX_TRANS_LEN];
                        num_bytes      = qupv3_handle[SPI_TX_TRANS_LEN];
                    }

                    SCP_DEBUG() << std::hex << "SLAVE WRITE - Data and Num Bytes:" << data << "  " << num_bytes;

                    val    = data; //Storing data into fifo
                    nbytes = num_bytes;
                    while (nbytes != 0) {
                        if (s->loopback == 0x1) {
                            read_queue.push(val & 0xff);
                            SCP_DEBUG() << "SLAVE Read queue data_byte:" << (val & 0xff);
                        } else {
                            write_queue.push(val & 0xff);
                            SCP_DEBUG() << "SLAVE Write queue data_byte:" << (val & 0xff);
                        }
                        val = (val >> 8);
                        nbytes--;
                    }
                    s->write_pos++;

                    /* Loop for debug purpose. Write up to 4 bytes from single FIFO on every request from software */
                    while ((count != FIFO_ENTRY_SIZE) && qupv3_handle[SPI_TX_TRANS_LEN] != 0) {
                        //ch = ( (value >> (count*8)) & 0xff );
                        //SCP_DEBUG()<<std::hex << "Byte value:"<<ch;
                        count++;
                        qupv3_handle[SPI_TX_TRANS_LEN] -= 1;
                    }

                    qupv3_handle[GENI_TX_FIFO_STATUS]--; //Decreasing word count
                    /* If no more data is left to write then set CMD_DONE irq bit */
                    if (qupv3_handle[SPI_TX_TRANS_LEN] == 0) {
                        s->write_done  = 0x1; //Write data received
                        s->write_start = 0x0;
                        s->write_pos   = 0x0;
                        SCP_INFO() << "****SLAVE TX Ended****";
                        /*    
                        SCP_DEBUG()<< "SLAVE write_fifo data: ");         //Print fifo content
                        showq(write_queue);
*/
                        //Clear IRQ status and line before setting bits
                        qupv3_handle[GENI_M_IRQ_STATUS] = 0x0;
                        irq->write(0);

                        if (s->opcode == SPI_TX_ONLY_OPCODE) { //Set IRQ for TX only opcode. For rest opcodes, CMD_DONE will be set post Rx completion
                            if (qupv3_handle[GENI_M_IRQ_ENABLE] & M_CMD_DONE) {
                                qupv3_handle[GENI_M_IRQ_STATUS] |= M_CMD_DONE;
                                qupv3_handle[GENI_M_CMD_0] = 0x0;
                                trigger_irq();
                            }
                        }
                    }
                }
            } else
                SCP_ERR() << "M_CMD_0 and UART_TX_LEN is not set properly";
            break;
        case GENI_TX_WATERMARK_REG:
            /* Write to TX watermark register */
            qupv3_handle[offset] = value;
            SCP_DEBUG() << std::hex << "SLAVE WRITE - Addr and value:" << offset << "  " << value;

            if (value == 0) { //Clear watermark bit from status
                qupv3_handle[GENI_M_IRQ_STATUS] &= ~(TX_FIFO_WATERMARK);
            }
            break;
        case GENI_RX_WATERMARK_REG:
        case GENI_RX_RFR_WATERMARK_REG:
            /* Write to RX watermark register */
            qupv3_handle[offset] = value;
            SCP_DEBUG() << std::hex << "SLAVE WRITE - Addr and value:" << offset << "  " << value;

            if (value == 0) { //Clear watermark bit from status
                qupv3_handle[GENI_M_IRQ_STATUS] &= ~(RX_FIFO_WATERMARK);
            }
            break;

        case DMA_TX_IRQ_CLR:
        case DMA_RX_IRQ_CLR:
        case SE_GSI_EVENT_EN:
        case SE_IRQ_EN:
        case DMA_GENERAL_CFG:
            /* Write registers based on offset */
            //Currently, DMA mode is not enabled.
            SCP_DEBUG() << std::hex << "SLAVE WRITE - Addr and value:" << offset << "  " << value;
            qupv3_handle[offset] = value;
            break;

        default:
            SCP_ERR() << std::hex << "Slave: Unhandled write(" << offset << "): " << value;
        }
    }
};
#endif //__QUPv3_SSPI__
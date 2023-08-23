/*
 * Quic Module qupv3_qupv3_se_wrapper_se0
 *
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version, or under the
 * Apache License, Version 2.0 (the "License”) at your discretion.
 *
 * SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA. You may obtain a copy of the Apache License at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef qupv3_qupv3_se_wrapper_se0_H
#define qupv3_qupv3_se_wrapper_se0_H

#include <systemc>
#include <cci_configuration>
#include <greensocs/gsutils/cciutils.h>
#include <scp/report.h>
#include <tlm>
#include <quic/gen_boilerplate/reg_model_maker.h>
#include <greensocs/gsutils/ports/initiator-signal-socket.h>
#include <greensocs/libgssync/async_event.h>
#include <greensocs/gsutils/ports/biflow-socket.h>
#include <greensocs/gsutils/cciutils.h>
#include <greensocs/gsutils/module_factory_registery.h>

/* this is because of the old qupv4 model elsewhere - should be removed !*/
#undef M_CMD_DONE
#undef RX_FIFO_LAST
#undef RX_LAST_BYTE_VALID
#undef RX_LAST
/*******/

#define GENI_RX_FIFO_MAX 4 /* Must be power of 2 */

/* clang-format off */
#define qupv3_qupv3_se_wrapper_se0_REGS                                             \
    (uint32_t, geni4_data.GENI_M_CMD0, GENI_M_CMD0),                                \
    (uint32_t, geni4_data.GENI_M_IRQ_STATUS, GENI_M_IRQ_STATUS),                    \
    (uint32_t, geni4_data.GENI_S_IRQ_STATUS, GENI_S_IRQ_STATUS),                    \
    (uint32_t, geni4_data.GENI_M_IRQ_CLEAR, GENI_M_IRQ_CLEAR),                      \
    (uint32_t, geni4_data.GENI_S_IRQ_CLEAR, GENI_S_IRQ_CLEAR),                      \
    (uint32_t, geni4_data.GENI_M_IRQ_ENABLE, GENI_M_IRQ_ENABLE),                    \
    (uint32_t, geni4_data.GENI_S_IRQ_ENABLE, GENI_S_IRQ_ENABLE),                    \
    (uint32_t, geni4_data.GENI_TX_FIFO, GENI_TX_FIFO),                              \
    (uint32_t, geni4_data.GENI_TX_FIFO_STATUS, GENI_TX_FIFO_STATUS),                \
    (uint32_t, geni4_image_regs.UART_TX_TRANS_LEN, UART_TX_TRANS_LEN),              \
    (uint32_t, geni4_image_regs.UART_MANUAL_RFR, UART_MANUAL_RFR),                  \
    (uint32_t, geni4_data.GENI_RX_FIFO, GENI_RX_FIFO),                              \
    (uint32_t, geni4_data.GENI_RX_FIFO_STATUS, GENI_RX_FIFO_STATUS),                \
    (uint32_t, geni4_data.GENI_RX_WATERMARK_REG, GENI_RX_WATERMARK_REG),            \
    (uint32_t, geni4_data.GENI_TX_WATERMARK_REG, GENI_TX_WATERMARK_REG),            \
    (uint32_t, geni4_cfg.GENI_FW_REVISION_RO, GENI_FW_REVISION_RO),                 \
    (uint32_t, geni4_cfg.GENI_STATUS, GENI_STATUS),                                 \
    (uint32_t, (GENI_M_IRQ_STATUS, TX_FIFO_WATERMARK), TX_FIFO_WATERMARK),          \
    (uint32_t, (GENI_M_IRQ_STATUS, M_CMD_DONE), M_CMD_DONE),                        \
    (uint32_t, (GENI_M_IRQ_STATUS, M_GP_SYNC_IRQ_0), M_GP_SYNC_IRQ_0),              \
    (uint32_t, (GENI_S_IRQ_STATUS, RX_FIFO_LAST), RX_FIFO_LAST),                    \
    (uint32_t, (GENI_RX_FIFO_STATUS, RX_LAST_BYTE_VALID), RX_LAST_BYTE_VALID),      \
    (uint32_t, (GENI_RX_FIFO_STATUS, RX_LAST), RX_LAST)
/* clang-format on */

class qupv3_qupv3_se_wrapper_se0 : public sc_core::sc_module
{
    SCP_LOGGER();
    gs::PrivateConfigurableBroker m_broker; // Should be a private broker for efficiency
    gs::json_zip_archive m_jza;
    bool loaded_ok;
    gs::json_module M;
    GSREG_DECLARE(qupv3_qupv3_se_wrapper_se0_REGS);

    uint rx_fifo_slot = 0;
    sc_core::sc_event irq_update;

    void update_irq()
    {
        bool level = ((GENI_M_IRQ_STATUS & GENI_M_IRQ_ENABLE) != 0) || ((GENI_S_IRQ_STATUS & GENI_S_IRQ_ENABLE) != 0);
        irq->write(level);
        SCP_DEBUG(())("Writing IRQ = {}", level);
        /* in the case that the driver has requested to clear the irq but there is still data
         * available, re-set them */
        if (level == 0 && rx_fifo_slot) {
            set_FIFO_STATUS();
            irq_update.notify();
        }
    }

    void set_FIFO_STATUS()
    {
        RX_LAST = rx_fifo_slot ? 1 : 0;
        RX_FIFO_LAST = RX_LAST;
        RX_LAST_BYTE_VALID = rx_fifo_slot;
    }
    void receive(tlm::tlm_generic_payload& txn, sc_core::sc_time& t)
    {
        uint8_t* data = txn.get_data_ptr();
        for (int i = 0; i < txn.get_data_length(); i++) {
            sc_assert(rx_fifo_slot < GENI_RX_FIFO_MAX);
            SCP_TRACE(())("adding {} to rx fifo slot {}", data[i], rx_fifo_slot);
            GENI_RX_FIFO[rx_fifo_slot++] = data[i];
        }
        set_FIFO_STATUS();
        irq_update.notify();
    }

public:
    tlm_utils::multi_passthrough_target_socket<qupv3_qupv3_se_wrapper_se0> target_socket;
    gs::biflow_socket<qupv3_qupv3_se_wrapper_se0> backend_socket;
    InitiatorSignalSocket<bool> irq;

    SC_HAS_PROCESS(qupv3_qupv3_se_wrapper_se0);

    qupv3_qupv3_se_wrapper_se0(sc_core::sc_module_name _name);

    void start_of_simulation()
    {
        /* NB writes should not happen before simulation starts, it's safe to do this here. */
        GENI_FW_REVISION_RO = 0x2ff; // implement UART protocol
    }

    void end_of_elaboration()
    {
        GENI_M_IRQ_ENABLE[M_CMD_DONE] = 1; // hack to handle broken driver (for now)
        backend_socket.can_receive_set(GENI_RX_FIFO_MAX);
        /* READ functionality */
        GENI_RX_FIFO.post_read([&](TXN()) {
            sc_assert(rx_fifo_slot > 0);
            for (int i = 0; i < GENI_RX_FIFO_MAX - 1; i++) {
                GENI_RX_FIFO[i] = GENI_RX_FIFO[i + 1];
            }
            rx_fifo_slot--;
            backend_socket.can_receive_more(1);
            set_FIFO_STATUS();
            irq_update.notify();
        });

        /* WRITE functionality */
        GENI_M_IRQ_ENABLE.post_write([&](TXN()) { irq_update.notify(); });
        GENI_S_IRQ_ENABLE.post_write([&](TXN()) { irq_update.notify(); });
        GENI_M_IRQ_CLEAR.post_write([&](TXN()) {
            GENI_M_IRQ_STATUS &= ~GENI_M_IRQ_CLEAR;
            irq_update.notify();
        });
        GENI_S_IRQ_CLEAR.post_write([&](TXN()) {
            GENI_S_IRQ_STATUS &= ~GENI_S_IRQ_CLEAR;
            irq_update.notify();
        });
        GENI_TX_FIFO.post_write([&](TXN(txn)) {
            sc_assert(txn.get_address() == 0); // must be first element in fifo.
            if ((GENI_M_CMD0 >> 27 == 1) && (UART_TX_TRANS_LEN > 0)) {
                // handle packing GENI_TX_PACKING_CFG0 GENI_TX_PACKING_CFG1
                for (int i = 0; (i < 4) && (UART_TX_TRANS_LEN > 0); i++) {
                    backend_socket.enqueue((unsigned char)(GENI_TX_FIFO[0] & 0xff));
                    GENI_TX_FIFO[0] >>= 8;
                    UART_TX_TRANS_LEN -= 1;
                }
                if (UART_TX_TRANS_LEN == 0) {
                    M_CMD_DONE = 1;
                    GENI_M_CMD0 = 0x0;
                }
                if (GENI_M_IRQ_ENABLE[M_GP_SYNC_IRQ_0]) {
                    sc_assert((GENI_M_IRQ_STATUS[M_GP_SYNC_IRQ_0]) == 0);
                    GENI_M_IRQ_STATUS[M_GP_SYNC_IRQ_0] = 1;
                }
                if (UART_TX_TRANS_LEN < GENI_TX_WATERMARK_REG) {
                    TX_FIFO_WATERMARK = 1;
                }
                irq_update.notify();
            } else {
                SCP_ERR(()) << "Error: M_CMD_0 and UART_TX_LEN is not set properly";
            }
        });
        UART_MANUAL_RFR.post_write([&](TXN()) {
            if ((UART_MANUAL_RFR & 0x80000003) == 0x80000002) {
                backend_socket.can_receive_set(0);
            } else {
                backend_socket.can_receive_set(GENI_RX_FIFO_MAX - rx_fifo_slot);
            }
        });
        GENI_RX_WATERMARK_REG.post_write([&](TXN()) { irq_update.notify(); });
        GENI_TX_WATERMARK_REG.post_write([&](TXN()) {
            GENI_M_IRQ_STATUS[TX_FIFO_WATERMARK] = 1;
            irq_update.notify();
        });
    }
};
GSC_MODULE_REGISTER(qupv3_qupv3_se_wrapper_se0);
#endif // qupv3_qupv3_se_wrapper_se0_H

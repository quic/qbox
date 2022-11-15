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

#ifndef __QUPv3_HEADER__
#define __QUPv3_HEADER__

#include <map>

/* QUPv3 : Sub modules */
typedef enum {
    QUP_BASE         = 0x0,
    GENI4_CFG        = 0x0,
    GENI4_IMAGE_REGS = 0x100,
    GENI4_DATA       = 0x600,
    QUPV3_SE_DMA     = 0xC00,
} qupv3_seg;

/* QUPv3 : Attach register names to the address */
typedef enum {
    GENI_M_CMD_0        = (GENI4_DATA),
    GENI_M_IRQ_STATUS   = ((GENI4_DATA) + 0x10),
    GENI_M_IRQ_CLEAR    = ((GENI4_DATA) + 0x18),
    GENI_TX_FIFO_0      = ((GENI4_DATA) + 0x100),
    GENI_TX_FIFO_STATUS = ((GENI4_DATA) + 0x200),
    UART_TX_TRANS_LEN   = ((GENI4_IMAGE_REGS) + 0x170),
    GENI_S_IRQ_STATUS   = ((GENI4_DATA) + 0x40),
    GENI_RX_FIFO_0      = ((GENI4_DATA) + 0x180),
    GENI_RX_FIFO_STATUS = ((GENI4_DATA) + 0x204),
    GENI_FW_REVISION_RO = ((GENI4_CFG) + 0x68),
    GENI_S_IRQ_CLEAR    = ((GENI4_DATA) + 0x48),

    // Unhandled register behaviours
    SE_GSI_EVENT_EN     = ((QUPV3_SE_DMA) + 0x218),
    GENI_S_IRQ_ENABLE   = ((GENI4_DATA) + 0x44),
    SE_IRQ_EN           = ((QUPV3_SE_DMA) + 0x21c),
    GENI_M_IRQ_EN_CLEAR = ((GENI4_DATA) + 0x20),
    GENI_S_IRQ_EN_CLEAR = ((GENI4_DATA) + 0x50),
    GENI_M_IRQ_EN_SET   = ((GENI4_DATA) + 0x1c),
    GENI_S_IRQ_EN_SET   = ((GENI4_DATA) + 0x4c),
    GENI_DMA_MODE_EN    = ((GENI4_IMAGE_REGS) + 0x158),
    GENI_S_CMD0         = ((GENI4_DATA) + 0x30),
    SE_HW_PARAM_0       = ((QUPV3_SE_DMA) + 0x224),
    UNKNOWN_TX_FIFO     = ((GENI4_DATA) + 0x144),

} qupv3_reg;

/* Defining bits for GENI_M_IRQ_STATUS reg */
#define TX_FIFO_WR_ERR (1 << 29)
#define M_CMD_DONE     (1 << 0)

/* Defining bits for GENI_S_IRQ_STATUS reg */
#define RX_FIFO_LAST      (1 << 27)
#define RX_FIFO_WATERMARK (1 << 26)

#define RX_LAST            (1 << 31)
#define RX_LAST_BYTE_VALID (1 << 28)

#endif

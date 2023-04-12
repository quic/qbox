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

/* Defining bits for GENI_M_IRQ_STATUS reg */
#define TX_FIFO_WATERMARK (1 << 30)
#define TX_FIFO_WR_ERR    (1 << 29)
#define M_CMD_ABORT       (1 << 5)
#define M_CMD_CANCEL      (1 << 4)
#define M_CMD_DONE        (1 << 0)

/* Defining bits for GENI_S_IRQ_STATUS reg */
#define RX_FIFO_LAST      (1 << 27)
#define RX_FIFO_WATERMARK (1 << 26)

/* Defining bits for SE_IRQ_EN reg */
#define GEN_M_EN (1 << 2)
#define GEN_S_EN (1 << 3)

/* Defining bits for GENI_M_CMD_0 reg */
#define M_CMD_OPCODE_START 27

/* Defining bits for GENI_M_CMD_CTRL_REG reg */
#define M_CMD_CTRL_ABORT  (1 << 1)
#define M_CMD_CTRL_CANCEL (1 << 2)

/* Defining bits for GENI_M_CMD_CTRL_REG reg */
#define RX_LAST_BYTE_VALID_OFF 28
#define RX_LAST                (1 << 31)

/* Defining bits for FIFO length */
#define FIFO_ENTRY_SIZE 4

/* SPI Opcodes */
#define SPI_TX_ONLY_OPCODE     1
#define SPI_RX_ONLY_OPCODE     2
#define SPI_FULL_DUPLEX_OPCODE 3
#define SPI_TX_RX_OPCODE       4
#define SPI_CS_ASSERT_OPCODE   8  //Not supported in slave mode
#define SPI_CS_DEASSERT_OPCODE 9  //Not supported in slave mode
#define SPI_SCLK_ONLY_OPCODE   10 //Not supported in slave mode

/* QUPv3 : Sub modules */
typedef enum {
    QUP_BASE         = 0x0,
    GENI4_CFG        = 0x0,
    GENI4_IMAGE_REGS = 0x100,
    GENI4_DATA       = 0x600,
    SE_DMA           = 0xC00,
} qupv3_seg;

/* QUPv3 : Attach register names to the address */
typedef enum {
    GENI_FORCE_DEFAULT_REG = ((QUP_BASE) + (GENI4_CFG) + 0x20),
    GENI_OUTPUT_CTRL       = ((QUP_BASE) + (GENI4_CFG) + 0x24),
    GENI_CGC_CTRL          = ((QUP_BASE) + (GENI4_CFG) + 0x28),
    GENI_SER_M_CLK_CFG     = ((QUP_BASE) + (GENI4_CFG) + 0x48),
    GENI_FW_REVISION       = ((QUP_BASE) + (GENI4_CFG) + 0x68),
    GENI_CLK_SEL           = ((QUP_BASE) + (GENI4_CFG) + 0x7C),
    GENI_DFS_IF_CFG        = ((QUP_BASE) + (GENI4_CFG) + 0x80),
    GENI_CFG_SEQ_START     = ((QUP_BASE) + (GENI4_CFG) + 0x84),
    GENI_CFG_STATUS        = ((QUP_BASE) + (GENI4_CFG) + 0x88),

    SPI_CPHA             = ((QUP_BASE) + (GENI4_IMAGE_REGS) + 0x124),
    SPI_LOOPBACK_CFG     = ((QUP_BASE) + (GENI4_IMAGE_REGS) + 0x12C),
    SPI_CPOL             = ((QUP_BASE) + (GENI4_IMAGE_REGS) + 0x130),
    GENI_CFG_REG80       = ((QUP_BASE) + (GENI4_IMAGE_REGS) + 0x140),
    SPI_DEMUX_OUTPUT_INV = ((QUP_BASE) + (GENI4_IMAGE_REGS) + 0x14C),
    SPI_DEMUX_SEL        = ((QUP_BASE) + (GENI4_IMAGE_REGS) + 0x150),
    GENI_DMA_MODE_EN     = ((QUP_BASE) + (GENI4_IMAGE_REGS) + 0x158),
    GENI_TX_PACKING_CFG0 = ((QUP_BASE) + (GENI4_IMAGE_REGS) + 0x160),
    GENI_TX_PACKING_CFG1 = ((QUP_BASE) + (GENI4_IMAGE_REGS) + 0x164),
    SPI_WORD_LEN         = ((QUP_BASE) + (GENI4_IMAGE_REGS) + 0x168),
    SPI_TX_TRANS_LEN     = ((QUP_BASE) + (GENI4_IMAGE_REGS) + 0x16C),
    UART_TX_TRANS_LEN    = ((QUP_BASE) + (GENI4_IMAGE_REGS) + 0x170),
    SPI_RX_TRANS_LEN     = ((QUP_BASE) + (GENI4_IMAGE_REGS) + 0x170),
    SPI_PRE_POST_CMD_DLY = ((QUP_BASE) + (GENI4_IMAGE_REGS) + 0x174),
    SPI_DELAYS_COUNTERS  = ((QUP_BASE) + (GENI4_IMAGE_REGS) + 0x178),
    GENI_RX_PACKING_CFG0 = ((QUP_BASE) + (GENI4_IMAGE_REGS) + 0x184),
    GENI_RX_PACKING_CFG1 = ((QUP_BASE) + (GENI4_IMAGE_REGS) + 0x188),
    SE_SPI_SLAVE_EN      = ((QUP_BASE) + (GENI4_IMAGE_REGS) + 0x1BC),

    GENI_M_CMD_0              = ((QUP_BASE) + (GENI4_DATA)),
    GENI_M_CMD_CTRL_REG       = ((QUP_BASE) + (GENI4_DATA) + 0x4),
    GENI_M_IRQ_STATUS         = ((QUP_BASE) + (GENI4_DATA) + 0x10),
    GENI_M_IRQ_ENABLE         = ((QUP_BASE) + (GENI4_DATA) + 0x14),
    GENI_M_IRQ_CLEAR          = ((QUP_BASE) + (GENI4_DATA) + 0x18),
    GENI_S_IRQ_STATUS         = ((QUP_BASE) + (GENI4_DATA) + 0x40),
    GENI_S_IRQ_ENABLE         = ((QUP_BASE) + (GENI4_DATA) + 0x44),
    GENI_S_IRQ_CLEAR          = ((QUP_BASE) + (GENI4_DATA) + 0x48),
    GENI_TX_FIFO_0            = ((QUP_BASE) + (GENI4_DATA) + 0x100),
    GENI_RX_FIFO_0            = ((QUP_BASE) + (GENI4_DATA) + 0x180),
    GENI_TX_FIFO_STATUS       = ((QUP_BASE) + (GENI4_DATA) + 0x200),
    GENI_RX_FIFO_STATUS       = ((QUP_BASE) + (GENI4_DATA) + 0x204),
    GENI_TX_WATERMARK_REG     = ((QUP_BASE) + (GENI4_DATA) + 0x20C),
    GENI_RX_WATERMARK_REG     = ((QUP_BASE) + (GENI4_DATA) + 0x210),
    GENI_RX_RFR_WATERMARK_REG = ((QUP_BASE) + (GENI4_DATA) + 0x214),

    DMA_TX_IRQ_CLR  = ((QUP_BASE) + (SE_DMA) + 0x44),
    DMA_RX_IRQ_CLR  = ((QUP_BASE) + (SE_DMA) + 0x144),
    SE_GSI_EVENT_EN = ((QUP_BASE) + (SE_DMA) + 0x218),
    SE_IRQ_EN       = ((QUP_BASE) + (SE_DMA) + 0x21C),
    HW_PARAM_0      = ((QUP_BASE) + (SE_DMA) + 0x224),
    HW_PARAM_1      = ((QUP_BASE) + (SE_DMA) + 0x228),
    DMA_GENERAL_CFG = ((QUP_BASE) + (SE_DMA) + 0x230),
} qupv3_reg;

#endif
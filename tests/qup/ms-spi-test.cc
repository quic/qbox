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

#include "quic/qup/spi/mspi-qupv3.h"
#include "quic/qup/spi/sspi-qupv3.h"

#include <greensocs/gsutils/tests/initiator-tester.h>
#include <greensocs/gsutils/tests/target-tester.h>
#include <greensocs/gsutils/tests/test-bench.h>

class TestSpi : public TestBench
{
    QUPv3_SpiM m_spi;
    QUPv3_SpiS s_spi;

public:
    InitiatorTester m_initiator;
    InitiatorTester n_initiator;

    TestSpi(const sc_core::sc_module_name& n)
        : TestBench(n)
        , m_spi("qup_spi1")
        , s_spi("qup_spi2")
        , m_initiator("initiator")
        , n_initiator("initiator") {
        m_initiator.socket.bind(m_spi.socket);
        n_initiator.socket.bind(s_spi.socket);

        m_spi.m2s_socket.bind(s_spi.s_socket);
        m_spi.irq.bind(m_spi.dummy_target);
        s_spi.irq.bind(s_spi.dummy_target);
    }
};

TEST_BENCH(TestSpi, SpiWrite) {
    int fifo_in_data, fifo_out_data = 0;
    int irq_status, fifo_status     = 0;
    int len, len1                   = 0x0;
    int word_count, val             = 0;

    std::cout << std::endl
              << ">>>>[Initialization]<<<<" << std::endl;
    /*Master Initialization*/
    m_initiator.do_write(SE_IRQ_EN, 0xF);                 /*Enable SE interrupts*/
    m_initiator.do_write(GENI_RX_RFR_WATERMARK_REG, 0xE); /*GENI_RX_RFR_WATERMARK_REG*/
    m_initiator.do_write(GENI_M_IRQ_ENABLE, 0x4c000001);  /*GENI_M_IRQ_ENABLE*/
    m_initiator.do_write(GENI_S_IRQ_ENABLE, 0x4c000001);  /*GENI_S_IRQ_ENABLE*/
    m_initiator.do_write(SPI_WORD_LEN, 0x4);              /*SPI_WORD_LEN*/
    m_initiator.do_write(SPI_DEMUX_SEL, 0x0);             /*SPI_DEMUX_SEL (slaveID)*/
    m_initiator.do_write(SPI_CPHA, 0x1);                  /*Clock Phase*/
    m_initiator.do_write(SPI_CPOL, 0x0);                  /*Clock Polarity*/
    m_initiator.do_write(SPI_LOOPBACK_CFG, 0x0);          /*Loopback cfg*/

    /*Slave Initialization*/
    n_initiator.do_write(SE_IRQ_EN, 0xF);                 /*Enable SE interrupts*/
    n_initiator.do_write(GENI_RX_RFR_WATERMARK_REG, 0xE); /*GENI_RX_RFR_WATERMARK_REG*/
    n_initiator.do_write(GENI_M_IRQ_ENABLE, 0x4c000001);  /*GENI_M_IRQ_ENABLE*/
    n_initiator.do_write(GENI_S_IRQ_ENABLE, 0x4c000001);  /*GENI_S_IRQ_ENABLE*/
    n_initiator.do_write(SPI_WORD_LEN, 0x4);              /*SPI_WORD_LEN*/
    n_initiator.do_write(SPI_DEMUX_SEL, 0x0);             /*SPI_DEMUX_SEL (slaveID)*/
    n_initiator.do_write(SPI_CPHA, 0x1);                  /*Clock Phase*/
    n_initiator.do_write(SPI_CPOL, 0x0);                  /*Clock Polarity*/
    n_initiator.do_write(SPI_LOOPBACK_CFG, 0x0);          /*Loopback cfg*/
    n_initiator.do_write(SE_SPI_SLAVE_EN, 0x1);           /*slave enable*/
    n_initiator.do_write(GENI_CFG_SEQ_START, 0x1);        /*Configuration sequence start*/

    std::cout << std::endl
              << ">>>>[TX PROCESS-1]<<<<" << std::endl;
    /*Transaction -TX (Master Write)*/
    /*Verify TX watermark and cmd_done interrupt set, FIFO wordcount*/
    len          = 0x1;
    fifo_in_data = 0x12;
    m_initiator.do_write(SPI_TX_TRANS_LEN, len);    /*SPI_TX_TRANS_LEN*/
    m_initiator.do_write(GENI_M_CMD_0, 0x08000000); /*M_CMD(TX only)*/

    m_initiator.do_read(GENI_M_IRQ_STATUS, irq_status);           /*GENI_M_IRQ_STATUS*/
    EXPECT_EQ(irq_status & TX_FIFO_WATERMARK, TX_FIFO_WATERMARK); /*Watermark set condition check*/

    m_initiator.do_read(GENI_TX_FIFO_STATUS, fifo_status); /*GENI_TX_FIFO_STATUS*/
    EXPECT_EQ(fifo_status & 0x0fffffff, len);              /*fifo wordcount set condition*/

    m_initiator.do_write(GENI_TX_FIFO_0, fifo_in_data); /*TX_FIFO*/

    m_initiator.do_read(GENI_M_IRQ_STATUS, irq_status); /*GENI_M_IRQ_STATUS*/
    EXPECT_EQ(irq_status & M_CMD_DONE, M_CMD_DONE);     /*cmd_done set condition*/

    m_initiator.do_write(GENI_M_IRQ_CLEAR, TX_FIFO_WATERMARK | M_CMD_DONE); /*GENI_M_IRQ_CLEAR*/
    /*Read data from slave*/
    n_initiator.do_write(SPI_RX_TRANS_LEN, len);    /*SPI_TX_TRANS_LEN*/
    n_initiator.do_write(GENI_M_CMD_0, 0x10000000); /*M_CMD(RX only)*/

    n_initiator.do_read(GENI_RX_FIFO_0, fifo_out_data);                     /*RX_FIFO*/
    EXPECT_EQ(fifo_out_data, fifo_in_data);                                 /*Data check*/
    n_initiator.do_write(GENI_M_IRQ_CLEAR, RX_FIFO_WATERMARK | M_CMD_DONE); /*GENI_M_IRQ_CLEAR*/

    std::cout << std::endl
              << ">>>>[TX PROCESS-2]<<<<" << std::endl;
    /*Transaction -TX (Master Write)*/
    /*verify Rx watermark, Rx FIFO status wordcount and data check in loopback mode*/
    len          = 0x2;
    word_count   = 0x1;
    fifo_in_data = 0x1234;

    m_initiator.do_write(SPI_LOOPBACK_CFG, 0x1); /*SPI_LOOPBACK_CFG*/

    m_initiator.do_write(SPI_TX_TRANS_LEN, len);    /*SPI_TX_TRANS_LEN*/
    m_initiator.do_write(SPI_RX_TRANS_LEN, len);    /*SPI_RX_TRANS_LEN*/
    m_initiator.do_write(GENI_M_CMD_0, 0x18000000); /*M_CMD (Full duplex)*/

    m_initiator.do_read(GENI_M_IRQ_STATUS, irq_status);           /*GENI_M_IRQ_STATUS*/
    EXPECT_EQ(irq_status & TX_FIFO_WATERMARK, TX_FIFO_WATERMARK); /*Tx Watermark set condition check*/

    m_initiator.do_read(GENI_TX_FIFO_STATUS, fifo_status); /*GENI_TX_FIFO_STATUS*/
    EXPECT_EQ(fifo_status & 0x0fffffff, word_count);       /*wordcount=0x1. fifo wordcount set condition*/

    m_initiator.do_write(GENI_TX_FIFO_0, fifo_in_data); /*TX_FIFO*/

    m_initiator.do_write(GENI_M_IRQ_CLEAR, TX_FIFO_WATERMARK); /*GENI_M_IRQ_CLEAR(clear Tx watermark)*/

    m_initiator.do_read(GENI_M_IRQ_STATUS, irq_status);           /*GENI_M_IRQ_STATUS*/
    EXPECT_EQ(irq_status & RX_FIFO_WATERMARK, RX_FIFO_WATERMARK); /*Rx Watermark set condition check*/

    m_initiator.do_read(GENI_RX_FIFO_STATUS, fifo_status); /*GENI_RX_FIFO_STATUS*/
    EXPECT_EQ(fifo_status & 0x1ffffff, word_count);        /*wordcount=1. fifo wordcount set condition*/

    m_initiator.do_read(GENI_RX_FIFO_0, fifo_out_data); /*RX_FIFO*/
    EXPECT_EQ(fifo_out_data, fifo_in_data);             /*Data check*/

    m_initiator.do_read(GENI_M_IRQ_STATUS, irq_status); /*GENI_M_IRQ_STATUS*/
    EXPECT_EQ(irq_status & M_CMD_DONE, M_CMD_DONE);     /*cmd_done set condition*/

    m_initiator.do_write(GENI_M_IRQ_CLEAR, RX_FIFO_LAST | RX_FIFO_WATERMARK | M_CMD_DONE); /*GENI_M_IRQ_CLEAR*/
    m_initiator.do_write(SPI_LOOPBACK_CFG, 0x0);                                           /*SPI_LOOPBACK_CFG*/

    std::cout << std::endl
              << ">>>>[TX PROCESS-3]<<<<" << std::endl;
    /*Transaction -TX (Master Write)*/
    /*Verify multi word scenario*/
    len          = 0x8;
    word_count   = 0x2;
    fifo_in_data = 0x12345678;
    m_initiator.do_write(SPI_TX_TRANS_LEN, len);    /*SPI_TX_TRANS_LEN*/
    m_initiator.do_write(GENI_M_CMD_0, 0x08000000); /*M_CMD*/

    m_initiator.do_read(GENI_TX_FIFO_STATUS, fifo_status); /*GENI_TX_FIFO_STATUS*/
    EXPECT_EQ(fifo_status & 0x0fffffff, word_count);       /*wordcount=0x2. fifo wordcount set condition*/

    m_initiator.do_write(GENI_TX_FIFO_0, fifo_in_data); /*TX_FIFO*/
    m_initiator.do_write(GENI_TX_FIFO_0, fifo_in_data); /*TX_FIFO*/

    m_initiator.do_read(GENI_M_IRQ_STATUS, irq_status); /*GENI_M_IRQ_STATUS*/
    EXPECT_EQ(irq_status & M_CMD_DONE, M_CMD_DONE);     /*cmd_done set condition*/

    m_initiator.do_write(GENI_M_IRQ_CLEAR, TX_FIFO_WATERMARK | M_CMD_DONE); /*GENI_M_IRQ_CLEAR*/
    /*Read data from slave*/
    n_initiator.do_write(SPI_RX_TRANS_LEN, len);    /*SPI_TX_TRANS_LEN*/
    n_initiator.do_write(GENI_M_CMD_0, 0x10000000); /*M_CMD(RX only)*/

    n_initiator.do_read(GENI_RX_FIFO_0, fifo_out_data); /*RX_FIFO*/
    EXPECT_EQ(fifo_out_data, fifo_in_data);             /*Data check*/
    n_initiator.do_read(GENI_RX_FIFO_0, fifo_out_data); /*RX_FIFO*/
    EXPECT_EQ(fifo_out_data, fifo_in_data);             /*Data check*/

    n_initiator.do_write(GENI_M_IRQ_CLEAR, RX_FIFO_WATERMARK | M_CMD_DONE); /*GENI_M_IRQ_CLEAR*/

    std::cout << std::endl
              << ">>>>[RX PROCESS-1]<<<<" << std::endl;
    /*Verify data received from slave is read by Master*/
    len          = 0x1;
    fifo_in_data = 0x10;
    /*Feed data in slave*/
    n_initiator.do_write(SPI_TX_TRANS_LEN, len);        /*SPI_TX_TRANS_LEN*/
    n_initiator.do_write(GENI_M_CMD_0, 0x08000000);     /*M_CMD(TX only)*/
    n_initiator.do_write(GENI_TX_FIFO_0, fifo_in_data); /*TX_FIFO*/

    n_initiator.do_read(GENI_M_IRQ_STATUS, irq_status); /*GENI_M_IRQ_STATUS*/
    EXPECT_EQ(irq_status & M_CMD_DONE, M_CMD_DONE);     /*cmd_done set condition*/

    n_initiator.do_write(GENI_M_IRQ_CLEAR, TX_FIFO_WATERMARK | M_CMD_DONE); /*GENI_M_IRQ_CLEAR*/
    /*initiate read from master*/
    m_initiator.do_write(SPI_RX_TRANS_LEN, len);    /*SPI_RX_TRANS_LEN*/
    m_initiator.do_write(GENI_M_CMD_0, 0x10000000); /*M_CMD(RX only)*/

    m_initiator.do_read(GENI_RX_FIFO_0, fifo_out_data); /*RX_FIFO*/
    EXPECT_EQ(fifo_out_data, fifo_in_data);             /*Data check*/

    m_initiator.do_read(GENI_M_IRQ_STATUS, irq_status); /*GENI_M_IRQ_STATUS*/
    EXPECT_EQ(irq_status & M_CMD_DONE, M_CMD_DONE);     /*cmd_done set condition*/

    m_initiator.do_write(GENI_M_IRQ_CLEAR, RX_FIFO_LAST | RX_FIFO_WATERMARK | M_CMD_DONE); /*GENI_M_IRQ_CLEAR*/

    std::cout << std::endl
              << ">>>>[RX PROCESS-2]<<<<" << std::endl;
    /*Verify multi word data received from slave is read by Master*/
    len          = 0x8;
    word_count   = 0x2;
    fifo_in_data = 0x10203040;
    /*Feed data in slave*/
    n_initiator.do_write(SPI_TX_TRANS_LEN, len);        /*SPI_TX_TRANS_LEN*/
    n_initiator.do_write(GENI_M_CMD_0, 0x08000000);     /*M_CMD(TX only)*/
    n_initiator.do_write(GENI_TX_FIFO_0, fifo_in_data); /*TX_FIFO*/
    n_initiator.do_write(GENI_TX_FIFO_0, fifo_in_data); /*TX_FIFO*/

    n_initiator.do_read(GENI_M_IRQ_STATUS, irq_status); /*GENI_M_IRQ_STATUS*/
    EXPECT_EQ(irq_status & M_CMD_DONE, M_CMD_DONE);     /*cmd_done set condition*/

    n_initiator.do_write(GENI_M_IRQ_CLEAR, TX_FIFO_WATERMARK | M_CMD_DONE); /*GENI_M_IRQ_CLEAR*/
    /*initiate read from master*/
    m_initiator.do_write(SPI_RX_TRANS_LEN, len);    /*SPI_RX_TRANS_LEN*/
    m_initiator.do_write(GENI_M_CMD_0, 0x10000000); /*M_CMD(RX only)*/

    m_initiator.do_read(GENI_RX_FIFO_STATUS, fifo_status); /*GENI_RX_FIFO_STATUS*/
    EXPECT_EQ(fifo_status & 0x1ffffff, word_count);        /*wordcount=2. fifo wordcount set condition*/

    m_initiator.do_read(GENI_RX_FIFO_0, fifo_out_data); /*RX_FIFO*/
    EXPECT_EQ(fifo_out_data, fifo_in_data);             /*Data check*/

    m_initiator.do_read(GENI_RX_FIFO_0, fifo_out_data); /*RX_FIFO*/
    EXPECT_EQ(fifo_out_data, fifo_in_data);             /*Data check*/

    m_initiator.do_read(GENI_M_IRQ_STATUS, irq_status);                                    /*GENI_M_IRQ_STATUS*/
    EXPECT_EQ(irq_status & M_CMD_DONE, M_CMD_DONE);                                        /*cmd_done set condition*/
    m_initiator.do_write(GENI_M_IRQ_CLEAR, RX_FIFO_LAST | RX_FIFO_WATERMARK | M_CMD_DONE); /*GENI_M_IRQ_CLEAR*/

    std::cout << std::endl
              << ">>>>[EXCHANGE PROCESS(Dual same length)-1]<<<<" << std::endl;
    /*verify xchange data from master with one byte data(Tx, Rx)*/
    len          = 0x1;
    val          = 0x12;
    fifo_in_data = 0xaa;

    /*Feed data in slave*/
    n_initiator.do_write(SPI_TX_TRANS_LEN, len);    /*SPI_TX_TRANS_LEN*/
    n_initiator.do_write(GENI_M_CMD_0, 0x08000000); /*M_CMD(TX only)*/

    n_initiator.do_write(GENI_TX_FIFO_0, fifo_in_data);                     /*TX_FIFO*/
    n_initiator.do_write(GENI_M_IRQ_CLEAR, TX_FIFO_WATERMARK | M_CMD_DONE); /*GENI_M_IRQ_CLEAR*/

    /*Initiate exchange transaction*/
    m_initiator.do_write(SPI_TX_TRANS_LEN, len); /*SPI_TX_TRANS_LEN*/
    m_initiator.do_write(SPI_RX_TRANS_LEN, len); /*SPI_RX_TRANS_LEN*/

    m_initiator.do_write(GENI_M_CMD_0, 0x18000000); /*M_CMD(Full Duplex)*/
    m_initiator.do_write(GENI_TX_FIFO_0, val);      /*TX_FIFO*/

    m_initiator.do_write(GENI_M_IRQ_CLEAR, TX_FIFO_WATERMARK); /*GENI_M_IRQ_CLEAR(clear Tx watermark)*/

    m_initiator.do_read(GENI_RX_FIFO_0, fifo_out_data); /*RX_FIFO*/
    EXPECT_EQ(fifo_out_data, fifo_in_data);             /*Data check*/

    m_initiator.do_write(GENI_M_IRQ_CLEAR, RX_FIFO_LAST | RX_FIFO_WATERMARK | M_CMD_DONE); /*GENI_M_IRQ_CLEAR*/

    /*Read data in slave*/
    n_initiator.do_write(SPI_RX_TRANS_LEN, len);    /*SPI_RX_TRANS_LEN*/
    n_initiator.do_write(GENI_M_CMD_0, 0x10000000); /*M_CMD(RX only)*/

    n_initiator.do_read(GENI_RX_FIFO_0, fifo_out_data);                                    /*RX_FIFO*/
    EXPECT_EQ(fifo_out_data, val);                                                         /*Data check*/
    n_initiator.do_write(GENI_M_IRQ_CLEAR, RX_FIFO_LAST | RX_FIFO_WATERMARK | M_CMD_DONE); /*GENI_M_IRQ_CLEAR*/

    std::cout << std::endl
              << ">>>>[EXCHANGE PROCESS(Dual same length)-2]<<<<" << std::endl;
    /*Verify xchange from master with 4bytes data (Tx, Rx)*/
    len          = 0x4;
    fifo_in_data = 0x11223344;
    val          = 0x12345678;

    /*Feed data in slave*/
    n_initiator.do_write(SPI_TX_TRANS_LEN, len);    /*SPI_TX_TRANS_LEN*/
    n_initiator.do_write(GENI_M_CMD_0, 0x08000000); /*M_CMD(Tx only)*/

    n_initiator.do_write(GENI_TX_FIFO_0, fifo_in_data);                     /*TX_FIFO*/
    n_initiator.do_write(GENI_M_IRQ_CLEAR, TX_FIFO_WATERMARK | M_CMD_DONE); /*GENI_M_IRQ_CLEAR*/

    /*Initiate exchange transaction*/
    m_initiator.do_write(SPI_TX_TRANS_LEN, len); /*SPI_TX_TRANS_LEN*/
    m_initiator.do_write(SPI_RX_TRANS_LEN, len); /*SPI_RX_TRANS_LEN*/

    m_initiator.do_write(GENI_M_CMD_0, 0x18000000); /*M_CMD*/
    m_initiator.do_write(GENI_TX_FIFO_0, val);      /*TX_FIFO*/

    m_initiator.do_write(GENI_M_IRQ_CLEAR, TX_FIFO_WATERMARK); /*GENI_M_IRQ_CLEAR(clear Tx watermark)*/

    m_initiator.do_read(GENI_RX_FIFO_0, fifo_out_data); /*RX_FIFO*/
    EXPECT_EQ(fifo_out_data, fifo_in_data);             /*Data check*/

    m_initiator.do_write(GENI_M_IRQ_CLEAR, RX_FIFO_LAST | RX_FIFO_WATERMARK | M_CMD_DONE); /*GENI_M_IRQ_CLEAR*/
    /*Read data in slave*/
    n_initiator.do_write(SPI_RX_TRANS_LEN, len);    /*SPI_RX_TRANS_LEN*/
    n_initiator.do_write(GENI_M_CMD_0, 0x10000000); /*M_CMD(RX only)*/

    n_initiator.do_read(GENI_RX_FIFO_0, fifo_out_data);                                    /*RX_FIFO*/
    EXPECT_EQ(fifo_out_data, val);                                                         /*Data check*/
    n_initiator.do_write(GENI_M_IRQ_CLEAR, RX_FIFO_LAST | RX_FIFO_WATERMARK | M_CMD_DONE); /*GENI_M_IRQ_CLEAR*/

    std::cout << std::endl
              << ">>>>[EXCHANGE PROCESS(Dual not same length)-3]<<<<" << std::endl;
    /*verify xchange from master with 4bytes Tx and 3bytes Rx*/
    len          = 0x3;
    len1         = 0x4;
    fifo_in_data = 0x112233;
    val          = 0x12345678;

    /*Feed data in slave*/
    n_initiator.do_write(SPI_TX_TRANS_LEN, len);    /*SPI_TX_TRANS_LEN*/
    n_initiator.do_write(GENI_M_CMD_0, 0x08000000); /*M_CMD*/

    n_initiator.do_write(GENI_TX_FIFO_0, fifo_in_data);                     /*TX_FIFO*/
    n_initiator.do_write(GENI_M_IRQ_CLEAR, TX_FIFO_WATERMARK | M_CMD_DONE); /*GENI_M_IRQ_CLEAR*/

    /*Initiate exchange transaction*/
    m_initiator.do_write(SPI_TX_TRANS_LEN, len1); /*SPI_TX_TRANS_LEN*/
    m_initiator.do_write(SPI_RX_TRANS_LEN, len);  /*SPI_RX_TRANS_LEN*/

    m_initiator.do_write(GENI_M_CMD_0, 0x18000000); /*M_CMD*/
    m_initiator.do_write(GENI_TX_FIFO_0, val);      /*TX_FIFO*/

    m_initiator.do_write(GENI_M_IRQ_CLEAR, TX_FIFO_WATERMARK); /*GENI_M_IRQ_CLEAR(clear Tx watermark)*/

    m_initiator.do_read(GENI_RX_FIFO_0, fifo_out_data); /*RX_FIFO*/
    EXPECT_EQ(fifo_out_data, fifo_in_data);             /*Data check*/

    m_initiator.do_write(GENI_M_IRQ_CLEAR, RX_FIFO_LAST | RX_FIFO_WATERMARK | M_CMD_DONE); /*GENI_M_IRQ_CLEAR*/

    /*Read data in slave*/
    n_initiator.do_write(SPI_RX_TRANS_LEN, len1);   /*SPI_RX_TRANS_LEN*/
    n_initiator.do_write(GENI_M_CMD_0, 0x10000000); /*M_CMD(RX only)*/

    n_initiator.do_read(GENI_RX_FIFO_0, fifo_out_data);                                    /*RX_FIFO*/
    EXPECT_EQ(fifo_out_data, val);                                                         /*Data check*/
    n_initiator.do_write(GENI_M_IRQ_CLEAR, RX_FIFO_LAST | RX_FIFO_WATERMARK | M_CMD_DONE); /*GENI_M_IRQ_CLEAR*/

    sc_core::wait(1, sc_core::SC_NS);
    sc_core::sc_stop();
}

int sc_main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

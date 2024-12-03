/*
 * Copyright (c) 2023, Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define SC_ALLOW_DEPRECATED_IEEE_API
#include <systemc>

#include "dmi-converter-bench.h"
#include <cci/utils/broker.h>

static uint8_t data[] = {
    0x0,  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xA,  0xB,  0xC,  0xD,  0xE,  0xF,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};

TEST_BENCH(DMIConverterTestBench, WriteReadDMIAllowed)
{
    SCP_INFO(()) << "read-write DMI allowed." << std::endl;

    do_write_read_check(0, (uint8_t*)&data, 8);
    print_dashes();
    do_write_read_check(0, (uint8_t*)&data, 12);
    print_dashes();
    do_write_read_check(12, (uint8_t*)&data + 12, 10);
    print_dashes();
}

TEST_BENCH(DMIConverterTestBench, ReadOnlyDMIAllowed)
{
    SCP_INFO(()) << "before (MEM_SIZE / 2) read-write dmi allowed, after (MEM_SIZE / 2) to 3 * "
                    "(MEM_SIZE / 4) read only DMI allowed."
                 << std::endl;

    do_write_read_check((MEM_SIZE / 2) - 8, (uint8_t*)&data, 8);
    print_dashes();
    do_write_read_check((MEM_SIZE / 2) - 8, (uint8_t*)&data, 16);
    print_dashes();
    do_write_read_check((MEM_SIZE / 2), (uint8_t*)&data,
                        12); //(MEM_SIZE / 2) +8 already exists in the cache because of successfull
                             // DMI read, but write DMI is not allowed
    print_dashes();
}

TEST_BENCH(DMIConverterTestBench, NoDMIAllowed)
{
    SCP_INFO(()) << "before  3 *(MEM_SIZE / 4) read only dmi allowed, after 3 * (MEM_SIZE / 4) to "
                    "MEM_SIZE DMI is not allowed."
                 << std::endl;

    do_write_read_check(3 * (MEM_SIZE / 4) - 16, (uint8_t*)&data, 8);
    print_dashes();
    do_write_read_check(3 * (MEM_SIZE / 4) - 8, (uint8_t*)&data + 8, 8);
    print_dashes();
    do_write_read_check(3 * (MEM_SIZE / 4) - 16, (uint8_t*)&data,
                        24); // 3 * (MEM_SIZE / 4) - 16 already exists in the cache because of successfull
                             // DMI read, but read-write DMI is not allowed after 3 * (MEM_SIZE / 4)
    print_dashes();
    do_write_read_check(3 * (MEM_SIZE / 4), (uint8_t*)&data + 16, 14);
    print_dashes();
}

TEST_BENCH(DMIConverterTestBench, UseByteEbale)
{
    SCP_INFO(()) << "Use Byte Enable." << std::endl;
    uint8_t byt[8] = { 0x00, 0xff, 0xff, 0x00, 0xff, 0x00, 0x00, 0xff };
    tlm::tlm_generic_payload trans;

    do_write_read_check_be(trans, 0, (uint8_t*)&data, 8, (uint8_t*)&byt, 8);
    print_dashes();
    do_write_read_check_be(trans, 0, (uint8_t*)&data, 8, (uint8_t*)&byt, 4);
    print_dashes();
    do_write_read_check_be(trans, (MEM_SIZE / 2) - 8, (uint8_t*)&data, 8, (uint8_t*)&byt, 8);
    print_dashes();
    do_write_read_check_be(trans, (MEM_SIZE / 2) - 4, (uint8_t*)&data, 8, (uint8_t*)&byt, 3);
    print_dashes();
    do_write_read_check_be(trans, (MEM_SIZE / 2) - 4, (uint8_t*)&data, 8, (uint8_t*)&byt, 5);
    print_dashes();
    do_write_read_check_be(trans, 3 * (MEM_SIZE / 4) - 8, (uint8_t*)&data, 8, (uint8_t*)&byt, 2);
    print_dashes();
    do_write_read_check_be(trans, 3 * (MEM_SIZE / 4) - 8, (uint8_t*)&data, 16, (uint8_t*)&byt, 7);
    print_dashes();
    do_write_read_check_be(trans, 3 * (MEM_SIZE / 4) - 4, (uint8_t*)&data, 16, (uint8_t*)&byt, 8);
    print_dashes();
}

int sc_main(int argc, char* argv[])
{
    cci_utils::consuming_broker broker("global_broker");
    cci_register_broker(broker);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
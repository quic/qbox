/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TIMEPRINTER_H
#define TIMEPRINTER_H
#include <systemc>
#include <scp/report.h>
#include <cci_configuration>
/**
 * @brief timeprinter: sc_module to print time at regular simulation intervals.
 * This is a simple sc_module which has no IO, and simply emits an 'info' message with the current sc_time and wall
 * clock time every interval_us microsecond. It uses the standard SCP_INFO report mechanism for output.
 *
 * NB to enable output, set the log_level for this module to at least 4
 *
 * @param interval_us
 * The number of microseconds of simulated time between prints
 *
 */
SC_MODULE (timeprinter) {
    SCP_LOGGER();
    cci::cci_param<uint64_t> interval_us;
    SC_CTOR (timeprinter) : interval_us("interval_us",1000000,"Interval between printing in microseconds")
        {
            SC_THREAD(print_time_stamp);
        }

    void print_time_stamp()
    {
        auto startRT = std::chrono::high_resolution_clock::now();

        while (1) {
            sc_core::sc_time now = sc_core::sc_time(std::chrono::duration_cast<std::chrono::microseconds>(
                                                        std::chrono::high_resolution_clock::now() - startRT)
                                                        .count(),
                                                    sc_core::SC_US);

            SCP_INFO(())("time stamp real time: {:.6}s", now.to_seconds());
            sc_core::wait(interval_us, sc_core::SC_US);
        }
    }
};

GSC_MODULE_REGISTER(timeprinter);
#endif //  TIMEPRINTER_H
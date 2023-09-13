/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BASIC_PLATFORM_REPORT_H
#define BASIC_PLATFORM_REPORT_H

#include <systemc>
#include <cinttypes>

namespace gs {

static const char* log_enabled = std::getenv("GS_LOG");
static const char* log_enabled_stdout = std::getenv("GS_LOG_STDOUT");
#define GS_LOG(...)                                                                                                   \
    do {                                                                                                              \
        static char __gs_log_buffer[100];                                                                             \
        if (gs::log_enabled) {                                                                                        \
            if (gs::log_enabled_stdout) {                                                                             \
                fprintf(stdout, "%s:%d ", __FILE__, __LINE__);                                                        \
                fprintf(stdout, __VA_ARGS__);                                                                         \
                fprintf(stdout, "\n");                                                                                \
            } else {                                                                                                  \
                snprintf(gs::__gs_log_buffer, sizeof(gs::__gs_log_buffer), __VA_ARGS__);                              \
                auto p = sc_core::sc_get_current_process_b();                                                         \
                if (p)                                                                                                \
                    sc_core::sc_report_handler::report(sc_core::SC_INFO, p->get_parent_object()->basename(),          \
                                                       gs::__gs_log_buffer, __FILE__, __LINE__);                      \
                else                                                                                                  \
                    sc_core::sc_report_handler::report(sc_core::SC_INFO, "non_module", gs::__gs_log_buffer, __FILE__, \
                                                       __LINE__);                                                     \
            }                                                                                                         \
        }                                                                                                             \
    } while (0)

} // namespace gs
#endif // BASIC_PLATFORM_REPORT_H

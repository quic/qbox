/*
 * Copyright (c) 2022 GreenSocs
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * You may obtain a copy of the Apache License at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef BASIC_PLATFORM_REPORT_H
#define BASIC_PLATFORM_REPORT_H

#include <systemc>
#include <cinttypes>

namespace gs {

static const char* log_enabled = std::getenv("GS_LOG");
static const char* log_enabled_stdout = std::getenv("GS_LOG_STDOUT");
static char __gs_log_buffer[100];
#define GS_LOG(...)                                                                              \
    do {                                                                                         \
        if (gs::log_enabled) {                                                                   \
            if (gs::log_enabled_stdout) {                                                        \
                fprintf(stdout, "%s:%d ", __FILE__, __LINE__);                                   \
                fprintf(stdout, __VA_ARGS__);                                                    \
                fprintf(stdout, "\n");                                                           \
            } else {                                                                             \
                snprintf(gs::__gs_log_buffer, sizeof(gs::__gs_log_buffer), __VA_ARGS__);         \
                auto p = sc_core::sc_get_current_process_b();                                    \
                if (p)                                                                           \
                    sc_core::sc_report_handler::report(sc_core::SC_INFO,                         \
                                                       p->get_parent_object()->basename(),       \
                                                       gs::__gs_log_buffer, __FILE__, __LINE__); \
                else                                                                             \
                    sc_core::sc_report_handler::report(sc_core::SC_INFO, "non_module",           \
                                                       gs::__gs_log_buffer, __FILE__, __LINE__); \
            }                                                                                    \
        }                                                                                        \
    } while (0)

} // namespace gs
#endif // BASIC_PLATFORM_REPORT_H

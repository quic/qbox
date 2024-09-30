/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GREENSOCS_MONITOR_H
#define _GREENSOCS_MONITOR_H

/**
 * PythonBinder is not meant to be used as a full systemc/TLM C++ -> python binder,
 * it is used to offload only part of the processing to a python script so that the powerful
 * and rich python language constructs and packages can be used to extend the capabilities
 * of QQVP. So, only the minimum required systemc/TLM classes/functions will have python bindings
 * using pybind11.
 */

#define SC_ALLOW_DEPRECATED_IEEE_API

#include <cci_configuration>
#include <systemc>
#include <tlm>
#include <scp/report.h>
#include <scp/helpers.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <ports/initiator-signal-socket.h>
#include <ports/target-signal-socket.h>
#include <tlm_sockets_buswidth.h>
#include <libgssync.h>
#include "crow.h"
#include <thread>
#include <future>
#include <qkmultithread.h>
#include <chrono>
#include <atomic>
#include <ports/biflow-socket.h>

#define MAX_ASIO_BUF_LEN (1024ULL * 16ULL)

namespace gs {

template <unsigned int BUSWIDTH = DEFAULT_TLM_BUSWIDTH>
class monitor : public sc_core::sc_module
{
    SCP_LOGGER();

    class biflow_ws
    {
        public:
        crow::websocket::connection* m_conn=nullptr;
        biflow_socket<biflow_ws> socket;
        std::string buffer;
        const char *name;

    public:
        void bind(biflow_bindable& other)
        {
            socket.bind(other);
            socket.can_receive_any();
        }
        void b_transport(tlm::tlm_generic_payload& txn, sc_core::sc_time& t)
        {
            /* NB we are ignoring the templated type here, as we're simply sending strings, if the type is not a
             * character, this will produce some sort of rubbish! */
            char* ptr = (char*)txn.get_data_ptr();
            std::string data(ptr, txn.get_data_length());
            buffer = (buffer + data);
            if (buffer.size()>1000) buffer=buffer.substr(buffer.size()-1000);
            if (m_conn) {
                m_conn->send_text(data);
            }
        }
        void enqueue(std::string data)
        {
            for (auto c : data) {
                socket.enqueue(c);
            }
        }
        biflow_ws(gs::biflow_multibindable& o, const char *n): socket(sc_core::sc_gen_unique_name("monitor_biflow_backend")), name(n)
        {
            socket.register_b_transport(this, &biflow_ws::b_transport);
            bind(o);
        }
        void set_conn(crow::websocket::connection* c) { m_conn = c; m_conn->send_text(buffer);}
        void clear_conn(){m_conn=nullptr;}
    };
    std::map<std::string, std::unique_ptr<biflow_ws>> biflows;

public:
    monitor(const sc_core::sc_module_name& nm);

    ~monitor();

private:
    void init_monitor();

    void before_end_of_elaboration() override;

    void end_of_elaboration() override;

    void start_of_simulation() override;

    void end_of_simulation() override;

public:
    cci::cci_param<uint32_t> p_server_port;
    cci::cci_param<std::string> p_html_doc_template_dir_path;
    cci::cci_param<std::string> p_html_doc_name;
    cci::cci_param<bool> p_use_html_presentation;
    crow::SimpleApp m_app;

private:
    std::future<void> m_app_future;
    gs::runonsysc m_sc;
    std::vector<tlm_quantumkeeper_multithread*> m_qks;
    double m_now;
    double m_qemu_timestamp_secs;
};
} // namespace gs

extern "C" void module_register();

#endif

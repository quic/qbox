/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file file.h
 * @brief file backend which support biflow socket
 */

#ifndef _GS_UART_BACKEND_FILE_H_
#define _GS_UART_BACKEND_FILE_H_
#include <systemc>
#include <tlm.h>
#include <tlm_utils/simple_target_socket.h>

#include <unistd.h>

#include <greensocs/libgssync/async_event.h>
#include <greensocs/gsutils/uutils.h>
#include <greensocs/gsutils/ports/biflow-socket.h>
#include <greensocs/gsutils/module_factory_registery.h>

#include <queue>
#include <stdlib.h>
#include <scp/report.h>
class CharBFBackendFile : public sc_core::sc_module
{
protected:
    cci::cci_param<std::string> p_read_file;
    cci::cci_param<std::string> p_write_file;
    cci::cci_param<unsigned int> p_baudrate;

private:
    FILE* r_file;
    FILE* w_file;
    double delay;
    SCP_LOGGER();

public:
    gs::biflow_socket<CharBFBackendFile> socket;
    sc_core::sc_event update_event;

#ifdef WIN32
#pragma message("CharBFBackendFile not yet implemented for WIN32")
#endif
    /**
     * CharBFBackendFile() - Construct the file-backend
     * @name: this backend's name
     * the paramters p_read_file, p_write_file and p_baudrate are CCI paramters
     */
    CharBFBackendFile(sc_core::sc_module_name name)
        : sc_core::sc_module(name)
        , p_read_file("read_file", "", "read file path")
        , p_write_file("write_file", "", "write file path")
        , p_baudrate("baudrate", 0, "number of bytes per second")
        , socket("biflow_socket")
    {
        SCP_TRACE(()) << "constructor";

        SC_THREAD(rcv_thread);
        sensitive << update_event;

        socket.register_b_transport(this, &CharBFBackendFile::writefn);
    }
    void start_of_simulation()
    {
        if (!p_read_file.get_value().empty()) {
            r_file = fopen(p_read_file.get_value().c_str(), "r");

            if (r_file == NULL) SCP_ERR(()) << "Error opening the file.\n";
            update_event.notify(sc_core::SC_ZERO_TIME);
        } else {
            SCP_ERR(()) << "Error reading the path of p_read_file.\n";
        }

        if (!p_write_file.get_value().empty()) {
            w_file = fopen(p_write_file.get_value().c_str(), "w");

            if (w_file == NULL) SCP_ERR(()) << "Error opening the file.\n";

            socket.can_receive_any();
        } else {
            SCP_ERR(()) << "Error reading the path of p_write_file.\n";
        }
    }
    void end_of_elaboration() {}

    void rcv_thread()
    {
        if (p_baudrate.get_value() == 0)
            delay = 0;
        else
            delay = (1.0 / p_baudrate.get_value());
        char c;
        while (fread(&c, sizeof(char), 1, r_file) == 1) {
            socket.enqueue(c);
            sc_core::wait(delay, sc_core::SC_SEC);
        }
        socket.enqueue(EOF);
        fclose(r_file);
    }

    void writefn(tlm::tlm_generic_payload& txn, sc_core::sc_time& t)
    {
        uint8_t* data = txn.get_data_ptr();
        for (int i = 0; i < txn.get_data_length(); i++) {
            size_t ret = fwrite(&data[i], sizeof(uint8_t), 1, w_file);
            if (ret != 1) {
                SCP_ERR(()) << "Error writing to the file.\n";
            }
        }
        fclose(w_file);
    }

    ~CharBFBackendFile() {}
};
GSC_MODULE_REGISTER(CharBFBackendFile);
#endif

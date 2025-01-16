/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GREENSOCS_BASE_COMPONENTS_REMOTE_H
#define _GREENSOCS_BASE_COMPONENTS_REMOTE_H

#include <cci_configuration>
#include <systemc>
#include <tlm>
#include <scp/report.h>

#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/multi_passthrough_initiator_socket.h>
#include <tlm_utils/multi_passthrough_target_socket.h>

#include <libgsutils.h>
#include <uutils.h>
#include <libgssync.h>
#include <cciutils.h>
#include <ports/initiator-signal-socket.h>
#include <ports/target-signal-socket.h>
#include <tlm-extensions/shmem_extension.h>
#include <module_factory_registery.h>
#include <module_factory_container.h>
#include <iomanip>
#include <unistd.h>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <algorithm>
#include <list>
#include <vector>
#include <atomic>
#include <future>
#include <queue>
#include <utility>
#include <type_traits>
#include <chrono>
#include <memory_services.h>

#include <rpc/client.h>
#include <rpc/rpc_error.h>
#include <rpc/server.h>
#include <rpc/this_handler.h>
#include <rpc/this_session.h>
#include <async_event.h>
#include <transaction_forwarder_if.h>
#include <tlm_sockets_buswidth.h>

#define GS_Process_Server_Port     "GS_Process_Server_Port"
#define GS_Process_Server_Port_Len 22
#define DECIMAL_PORT_NUM_STR_LEN   20
#define DECIMAL_PID_T_STR_LEN      20
#define RPC_TIMEOUT                500

namespace gs {

// #define DMICACHE switchthis on - then you need a mutex
/* rpc pass through should pass through ONE forward connection ? */

template <unsigned int BUSWIDTH = DEFAULT_TLM_BUSWIDTH>
class PassRPC : public sc_core::sc_module, public transaction_forwarder_if<PASS>
{
    SCP_LOGGER();
    using MOD = PassRPC<BUSWIDTH>;

    static std::string txn_str(tlm::tlm_generic_payload& trans)
    {
        std::stringstream info;

        const char* cmd = "unknown";
        switch (trans.get_command()) {
        case tlm::TLM_IGNORE_COMMAND:
            cmd = "ignore";
            break;
        case tlm::TLM_WRITE_COMMAND:
            cmd = "write";
            break;
        case tlm::TLM_READ_COMMAND:
            cmd = "read";
            break;
        }

        info << " " << cmd << " to address "
             << "0x" << std::hex << trans.get_address();

        info << " len:" << trans.get_data_length();
        unsigned char* ptr = trans.get_data_ptr();
        info << " returned with data 0x";
        for (int i = trans.get_data_length(); i; i--) {
            info << std::setw(2) << std::setfill('0') << std::hex << (unsigned int)(ptr[i - 1]);
        }
        info << " status:" << trans.get_response_status() << " ";
        if (trans.is_dmi_allowed()) info << " DMI OK ";
        for (int i = 0; i < tlm::max_num_extensions(); i++) {
            if (trans.get_extension(i)) {
                info << " extn " << i;
            }
        }
        return info.str();
    }

    using str_pairs = std::vector<std::pair<std::string, std::string>>;
#ifdef DMICACHE
    /* Handle local DMI cache */

    std::map<uint64_t, tlm::tlm_dmi> m_dmi_cache;
    tlm::tlm_dmi* in_cache(uint64_t address)
    {
        if (m_dmi_cache.size() > 0) {
            auto it = m_dmi_cache.upper_bound(address);
            if (it != m_dmi_cache.begin()) {
                it = std::prev(it);
                if ((address >= it->second.get_start_address()) && (address <= it->second.get_end_address())) {
                    return &(it->second);
                }
            }
        }
        return nullptr;
    }
    void cache_clean(uint64_t start, uint64_t end)
    {
        auto it = m_dmi_cache.upper_bound(start);

        if (it != m_dmi_cache.begin()) {
            /*
             * Start with the preceding region, as it may already cross the
             * range we must invalidate.
             */
            it--;
        }

        while (it != m_dmi_cache.end()) {
            tlm::tlm_dmi& r = it->second;

            if (r.get_start_address() > end) {
                /* We've got out of the invalidation range */
                break;
            }

            if (r.get_end_address() < start) {
                /* We are not in yet */
                it++;
                continue;
            }
            it = m_dmi_cache.erase(it);
        }
    }
#endif
    /* RPC structure for TLM_DMI */
    struct tlm_dmi_rpc {
        std::string m_shmem_fn;
        size_t m_shmem_size;
        uint64_t m_shmem_offset;

        uint64_t m_dmi_start_address;
        uint64_t m_dmi_end_address;
        int m_dmi_access; /*tlm::tlm_dmi::dmi_access_e */
        double m_dmi_read_latency;
        double m_dmi_write_latency;

        MSGPACK_DEFINE_ARRAY(m_shmem_fn, m_shmem_size, m_shmem_offset, m_dmi_start_address, m_dmi_end_address,
                             m_dmi_access, m_dmi_read_latency, m_dmi_write_latency);

        void from_tlm(tlm::tlm_dmi& other, ShmemIDExtension* shm)
        {
            m_shmem_fn = shm->m_memid;
            m_shmem_size = shm->m_size;
            //            SCP_DEBUG(()) << "DMI from tlm Size "<<m_shmem_size;

            m_shmem_offset = (uint64_t)(other.get_dmi_ptr()) - shm->m_mapped_addr;
            m_dmi_start_address = other.get_start_address();
            m_dmi_end_address = other.get_end_address();
            m_dmi_access = other.get_granted_access();
            m_dmi_read_latency = other.get_read_latency().to_seconds();
            m_dmi_write_latency = other.get_write_latency().to_seconds();
        }

        void to_tlm(tlm::tlm_dmi& other)
        {
            if (m_shmem_size == 0) return;

            //            SCP_DEBUG(()) << "DMI to_tlm Size "<<m_shmem_size;

            other.set_dmi_ptr(m_shmem_offset + MemoryServices::get().map_mem_join(m_shmem_fn.c_str(), m_shmem_size));
            other.set_start_address(m_dmi_start_address);
            other.set_end_address(m_dmi_end_address);
            other.set_granted_access((tlm::tlm_dmi::dmi_access_e)m_dmi_access);
            other.set_read_latency(sc_core::sc_time(m_dmi_read_latency, sc_core::SC_SEC));
            other.set_write_latency(sc_core::sc_time(m_dmi_write_latency, sc_core::SC_SEC));
        }
    };

    struct tlm_generic_payload_rpc {
        sc_dt::uint64 m_address;
        int m_command;
        unsigned int m_length;
        int m_response_status;
        bool m_dmi;
        unsigned int m_byte_enable_length;
        unsigned int m_streaming_width;
        int m_gp_option;

        double m_sc_time;
        double m_quantum_time;

        std::vector<unsigned char> m_data;
        std::vector<unsigned char> m_byte_enable;
        // extensions will not be carried
        MSGPACK_DEFINE_ARRAY(m_address, m_command, m_length, m_response_status, m_dmi, m_byte_enable_length,
                             m_streaming_width, m_gp_option, m_sc_time, m_quantum_time, m_data, m_byte_enable);

        void from_tlm(tlm::tlm_generic_payload& other)
        {
            m_command = other.get_command();
            m_address = other.get_address();
            m_length = other.get_data_length();
            m_response_status = other.get_response_status();
            m_byte_enable_length = other.get_byte_enable_length();
            m_streaming_width = other.get_streaming_width();
            m_gp_option = other.get_gp_option();
            m_dmi = other.is_dmi_allowed();
            unsigned char* data_ptr = other.get_data_ptr();
            if (m_length && data_ptr) {
                m_data.resize(m_length);
                std::copy(data_ptr, data_ptr + m_length, m_data.begin());
            } else {
                m_length = 0;
                m_data.clear();
            }
            unsigned char* byte_enable_ptr = other.get_byte_enable_ptr();
            if (m_byte_enable_length && byte_enable_ptr) {
                m_byte_enable.resize(m_byte_enable_length);
                std::copy(byte_enable_ptr, byte_enable_ptr + m_byte_enable_length, m_byte_enable.begin());
            } else {
                m_byte_enable_length = 0;
                m_byte_enable.clear();
            }
        }
        void deep_copy_to_tlm(tlm::tlm_generic_payload& other)
        {
            other.set_command((tlm::tlm_command)(m_command));
            other.set_address(m_address);
            other.set_data_length(m_length);
            other.set_response_status((tlm::tlm_response_status)(m_response_status));
            other.set_byte_enable_length(m_byte_enable_length);
            other.set_streaming_width(m_streaming_width);
            other.set_gp_option((tlm::tlm_gp_option)(m_gp_option));
            other.set_dmi_allowed(m_dmi);
            if (!m_length) {
                other.set_data_ptr(nullptr);
            } else {
                other.set_data_ptr(reinterpret_cast<unsigned char*>(m_data.data()));
            }
            if (!m_byte_enable_length) {
                other.set_byte_enable_ptr(nullptr);
            } else {
                other.set_byte_enable_ptr(reinterpret_cast<unsigned char*>(m_byte_enable.data()));
            }
        }

        void update_to_tlm(tlm::tlm_generic_payload& other)
        {
            tlm::tlm_generic_payload tmp; // make use of TLM's built in update
            if (!m_length) {
                tmp.set_data_ptr(nullptr);
            } else {
                tmp.set_data_ptr(reinterpret_cast<unsigned char*>(m_data.data()));
            }

            if (!m_byte_enable_length) {
                other.set_byte_enable_ptr(nullptr);
            } else {
                tmp.set_byte_enable_ptr(reinterpret_cast<unsigned char*>(m_byte_enable.data()));
            }
            tmp.set_data_length(m_length);
            tmp.set_byte_enable_length(m_byte_enable_length);
            tmp.set_response_status((tlm::tlm_response_status)m_response_status);
            tmp.set_dmi_allowed(m_dmi);
            other.update_original_from(tmp, m_byte_enable_length > 0);
        }
    };

    cci::cci_broker_handle m_broker;
    str_pairs m_cci_db;
    std::mutex m_cci_db_mut;

    class initiator_socket_spying
        : public tlm_utils::simple_initiator_socket_b<MOD, BUSWIDTH, tlm::tlm_base_protocol_types,
                                                      sc_core::SC_ZERO_OR_MORE_BOUND>
    {
        using socket_type = tlm_utils::simple_initiator_socket_b<MOD, BUSWIDTH, tlm::tlm_base_protocol_types,
                                                                 sc_core::SC_ZERO_OR_MORE_BOUND>;
        using typename socket_type::base_target_socket_type; // tlm_utils::simple_initiator_socket<MOD,
                                                             // BUSWIDTH>::base_target_socket_type;

        const std::function<void(std::string)> register_cb;

    public:
        initiator_socket_spying(const char* name, const std::function<void(std::string)>& f)
            : socket_type::simple_initiator_socket_b(name), register_cb(f)
        {
        }

        void bind(base_target_socket_type& socket)
        {
            socket_type::bind(socket);
            register_cb(socket.get_base_export().name());
        }

        // hierarchial binding
        void bind(tlm::tlm_initiator_socket<BUSWIDTH>& socket)
        {
            socket_type::bind(socket);
            register_cb(socket.get_base_port().name());
        }
    };

    /* NB use the EXPORT name, so as not to be hassled by the _port_0*/
    std::string nameFromSocket(std::string s) { return s; }

    void remote_register_boundto(std::string s) { SCP_DEBUG(()) << "Binding: " << s; }

public:
    // NB there is a 'feature' in multi passthrough sockets, the 'name' is always returned as the
    // name of the socket itself, in our case "target_socket_0". This means that address lookup will
    // only work for the FIRST such socket, all others will require a 'pass' or should be driven
    // from models that dont use the CCI address map info.
    // We can fix this using a template and vector of sockets....
    using tlm_target_socket = tlm_utils::simple_target_socket_tagged_b<MOD, BUSWIDTH, tlm::tlm_base_protocol_types,
                                                                       sc_core::SC_ZERO_OR_MORE_BOUND>;

    sc_core::sc_vector<tlm_target_socket> target_sockets;
    sc_core::sc_vector<initiator_socket_spying> initiator_sockets;

    sc_core::sc_vector<InitiatorSignalSocket<bool>> initiator_signal_sockets;
    sc_core::sc_vector<TargetSignalSocket<bool>> target_signal_sockets;

    cci::cci_param<int> p_cport;
    cci::cci_param<int> p_sport;
    cci::cci_param<std::string> p_exec_path;
    bool m_is_local;
    cci::cci_param<std::string> p_sync_policy;
    cci::cci_param<uint32_t> p_tlm_initiator_ports_num;
    cci::cci_param<uint32_t> p_tlm_target_ports_num;
    cci::cci_param<uint32_t> p_initiator_signals_num;
    cci::cci_param<uint32_t> p_target_signals_num;

private:
    rpc::client* client = nullptr;
    rpc::server* server = nullptr;
    int m_child_pid = 0;
    ProcAliveHandler pahandler;
    std::condition_variable is_client_connected;
    std::condition_variable is_sc_status_set;
    std::mutex client_conncted_mut;
    std::mutex sc_status_mut;
    std::mutex stop_mutex;
    std::atomic_bool cancel_waiting;
    std::thread::id sc_tid;
    std::queue<std::pair<int, bool>> sig_queue;
    std::mutex sig_queue_mut;
    std::vector<const char*> m_remote_args;
    sc_core::sc_status m_remote_status = static_cast<sc_core::sc_status>(0);

    int targets_bound = 0;

    // std::shared_ptr<gs::tlm_quantumkeeper_extended> m_qk;
    gs::runonsysc m_sc;
    gs::ModuleFactory::ContainerBase* m_container;

    class trans_waiter
    {
    private:
        std::string m_name;
        std::queue<std::function<void()>> notifiers;
        std::thread notifier_thread;
        std::atomic_bool is_stopped;
        std::atomic_bool is_started;

    public:
        std::vector<gs::async_event> data_ready_events;
        std::vector<sc_core::sc_event> port_available_events;
        std::vector<bool> is_port_busy;
        std::condition_variable is_rpc_execed;
        std::mutex start_stop_mutex;
        std::mutex rpc_execed_mut;
        /**
         * FIXME: this is a temp solution for making the b_transport reentrant.
         * This thread should wait for the future and notify the systemc waiting
         * thread by executing the enqueued notifiers.
         * This solution should be revisted in future.
         */
    private:
        void notifier_task()
        {
            while (!is_stopped) {
                std::unique_lock<std::mutex> ul(rpc_execed_mut);
                is_rpc_execed.wait_for(ul, std::chrono::milliseconds(RPC_TIMEOUT),
                                       [this]() { return (!notifiers.empty() || is_stopped); });
                while (!notifiers.empty()) {
                    notifiers.front()();
                    notifiers.pop();
                }
                ul.unlock();
            }
        }

    public:
        trans_waiter(std::string p_name, uint32_t ev_num)
            : m_name(p_name)
            , data_ready_events(ev_num)
            , port_available_events(ev_num)
            , is_port_busy(ev_num, false)
            , is_stopped(false)
            , is_started(false)
        {
        }
        void start()
        {
            {
                std::lock_guard<std::mutex> start_lg(start_stop_mutex);
                if (is_started) return;
                is_started = true;
            }

            notifier_thread = std::thread(&trans_waiter::notifier_task, this);
        }

        void stop()
        {
            {
                std::lock_guard<std::mutex> stop_lg(start_stop_mutex);
                if (is_stopped || !is_started) return;
                is_stopped = true;
            }
            {
                std::lock_guard<std::mutex> lg(rpc_execed_mut);
                is_rpc_execed.notify_one();
            }

            if (notifier_thread.joinable()) notifier_thread.join();
        }

        ~trans_waiter() { stop(); }

        void enqueue_notifier(std::function<void()> notifier) { notifiers.push(notifier); }
    };

    std::unique_ptr<trans_waiter> btspt_waiter;

    template <typename... Args>
    std::future<RPCLIB_MSGPACK::object_handle> do_rpc_async_call(std::string const& func_name, Args... args)
    {
        if (!client) {
            stop_and_exit();
        }
        std::future<RPCLIB_MSGPACK::object_handle> ret;
        try {
            ret = client->async_call(func_name, std::forward<Args>(args)...);
        } catch (const std::future_error& e) {
            SCP_DEBUG(()) << name() << " PassRPC::do_rpc_async_call() Connection with remote is closed: " << e.what();
            stop_and_exit();
        } catch (...) {
            SCP_DEBUG(()) << name() << " PassRPC::do_rpc_async_call() Unknown error!";
            stop_and_exit();
        }
        return ret;
    }

    template <typename T>
    T do_rpc_async_get(std::future<RPCLIB_MSGPACK::object_handle> fut)
    {
        T ret;
        try {
            ret = do_rpc_as<T>(fut.get());
        } catch (const std::future_error& e) {
            SCP_DEBUG(()) << name() << " PassRPC::do_rpc_async_get() RPC future is corrupted: " << e.what();
            stop_and_exit();
        } catch (...) {
            SCP_DEBUG(()) << name() << " PassRPC::do_rpc_async_get() Unknown error!";
            stop_and_exit();
        }
        return ret;
    }

    template <typename T>
    T do_rpc_as(RPCLIB_MSGPACK::object_handle handle)
    {
        T ret;
        try {
            ret = handle.template as<T>();
        } catch (...) {
            SCP_DEBUG(()) << name() << " PassRPC::do_rpc_as() RPC remote value is corrupted!";
            stop_and_exit();
        }
        return ret;
    }

    template <typename... Args>
    RPCLIB_MSGPACK::object_handle do_rpc_call(std::string const& func_name, Args... args)
    {
        if (!client) {
            stop_and_exit();
        }
        RPCLIB_MSGPACK::object_handle ret;
        try {
            ret = client->call(func_name, std::forward<Args>(args)...);
        }

        catch (const std::future_error& e) {
            SCP_DEBUG(()) << name() << " PassRPC::do_rpc_call() Connection with remote is closed: " << e.what();
            stop_and_exit();
        } catch (...) {
            SCP_DEBUG(()) << name() << " PassRPC::do_rpc_call() Unknown error!";
            stop_and_exit();
        }

        return ret;
    }

    bool is_local_mode() { return (m_container && m_is_local); }

    void fw_b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay) override
    {
        SCP_DEBUG(()) << "calling b_transport on initiator_socket_" << id << " " << scp::scp_txn_tostring(trans);
        initiator_sockets[id]->b_transport(trans, delay);
        SCP_DEBUG(()) << "return from b_transport on initiator_socket_" << id << " " << scp::scp_txn_tostring(trans);
    }

    unsigned int fw_transport_dbg(int id, tlm::tlm_generic_payload& trans) override
    {
        SCP_DEBUG(()) << "calling transport_dbg on initiator_socket_" << id << " " << scp::scp_txn_tostring(trans);
        unsigned int ret = initiator_sockets[id]->transport_dbg(trans);
        SCP_DEBUG(()) << "return from transport_dbg on initiator_socket_" << id << " " << scp::scp_txn_tostring(trans);
        return ret;
    }

    bool fw_get_direct_mem_ptr(int id, tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data) override
    {
        SCP_DEBUG(()) << "calling get_direct_mem_ptr on initiator_socket_" << id << " " << scp::scp_txn_tostring(trans);
        bool ret = initiator_sockets[id]->get_direct_mem_ptr(trans, dmi_data);
        SCP_DEBUG(()) << "return from get_direct_mem_ptr on initiator_socket_" << id << " "
                      << " RET: " << std::boolalpha << ret << " " << scp::scp_txn_tostring(trans)
                      << " IS_READ_ALLOWED: " << std::boolalpha << dmi_data.is_read_allowed() << " "
                      << " IS_WRITE_ALLOWED: " << std::boolalpha << dmi_data.is_write_allowed();
        return ret;
    }

    void fw_invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end) override
    {
        SCP_DEBUG(()) << " " << name() << " invalidate_direct_mem_ptr "
                      << " start address 0x" << std::hex << start << " end address 0x" << std::hex << end;
        for (int i = 0; i < target_sockets.size(); i++) {
            target_sockets[i]->invalidate_direct_mem_ptr(start, end);
        }
    }

    void fw_handle_signal(int id, bool value) override
    {
        SCP_DEBUG(()) << "calling handle_signal on initiator_signal_socket_" << id << " value: " << std::boolalpha
                      << value;
        initiator_signal_sockets[id]->write(value);
    }

    /* b_transport interface */
    void b_transport(int id, tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        if (is_local_mode()) {
            m_container->fw_b_transport(id, trans, delay);
            return;
        }

        while (btspt_waiter->is_port_busy[id]) {
            sc_core::wait(btspt_waiter->port_available_events[id]);
        }
        btspt_waiter->is_port_busy[id] = true;

        tlm_generic_payload_rpc t;
        tlm_generic_payload_rpc r;
        double time = sc_core::sc_time_stamp().to_seconds();

        uint64_t addr = trans.get_address();
        // If we have a locally cached DMI, use it!
#ifdef DMICACHE
        auto c = in_cache(addr);
        if (c) {
            uint64_t len = trans.get_data_length();
            if (addr >= c->get_start_address() && addr + len <= c->get_end_address()) {
                switch (trans.get_command()) {
                case tlm::TLM_IGNORE_COMMAND:
                    break;
                case tlm::TLM_WRITE_COMMAND:
                    memcpy(c->get_dmi_ptr() + (addr - c->get_start_address()), trans.get_data_ptr(), len);
                    break;
                case tlm::TLM_READ_COMMAND:
                    memcpy(trans.get_data_ptr(), c->get_dmi_ptr() + (addr - c->get_start_address()), len);
                    break;
                }
                trans.set_dmi_allowed(true);
                trans.set_response_status(tlm::TLM_OK_RESPONSE);
                return;
            }
        }
#endif
        t.from_tlm(trans);
        t.m_quantum_time = delay.to_seconds();
        t.m_sc_time = sc_core::sc_time_stamp().to_seconds();

        //        SCP_DEBUG(()) << name() << " b_transport socket ID " << id << " From_tlm " <<
        //        txn_str(trans); SCP_DEBUG(()) << getpid() <<" IS THE b_ RPC PID " <<
        //        std::this_thread::get_id() <<" is the thread ID";
        /*tlm_generic_payload_rpc r = client->call("b_tspt", id, t)
                                        .template as<tlm_generic_payload_rpc>();*/
        /**
         * FIXME: this is a temp solution for making the b_transport reentrant.
         * remove the quantumkeeper and make b_tspt RPC async call, wait for the future
         * from the async_call in a separate thread, then notify the waiting systemc thread.
         * This solution should be revisted in future.
         */
        if (std::this_thread::get_id() == sc_tid && sc_core::sc_get_status() >= sc_core::sc_status::SC_RUNNING &&
            sc_core::sc_get_curr_process_kind() != sc_core::sc_curr_proc_kind::SC_NO_PROC_) {
            SCP_DEBUG(()) << name() << " B_TSPT handle reentrancy, sc_get_curr_simcontext " << sc_get_curr_simcontext()
                          << " SC current process kind = " << sc_core::sc_get_curr_process_kind();
            btspt_waiter->start();

            std::unique_lock<std::mutex> ul(btspt_waiter->rpc_execed_mut);
            btspt_waiter->enqueue_notifier([&]() {
                r = do_rpc_as<tlm_generic_payload_rpc>(do_rpc_call("b_tspt", id, t));
                btspt_waiter->data_ready_events[id].async_notify();
            });
            btspt_waiter->is_rpc_execed.notify_one();
            ul.unlock();
            SCP_DEBUG(()) << name() << " B_TSPT wait for event, sc_get_curr_simcontext " << sc_get_curr_simcontext()
                          << " SC current process kind = " << sc_core::sc_get_curr_process_kind();
            if (sc_core::sc_get_curr_process_kind() != sc_core::sc_curr_proc_kind::SC_METHOD_PROC_) {
                sc_core::wait(btspt_waiter->data_ready_events[id]); // systemc wait
            } else {
                SCP_FATAL(()) << name() << " b_transport was called from the context of SC_METHOD!";
            }
        } else {
            r = do_rpc_as<tlm_generic_payload_rpc>(do_rpc_call("b_tspt", id, t));
        }

        r.update_to_tlm(trans);
        delay = sc_core::sc_time(r.m_quantum_time, sc_core::SC_SEC);
        sc_core::sc_time other_time = sc_core::sc_time(r.m_sc_time, sc_core::SC_SEC);
        //        m_qk->set(other_time+delay);
        //        m_qk->sync();
        //        SCP_DEBUG(()) << name() << " update_to_tlm " << txn_str(trans);
        //        SCP_DEBUG(()) << name() << " b_transport socket ID " << id << " returned " <<
        //        txn_str(trans);
        btspt_waiter->is_port_busy[id] = false;
        btspt_waiter->port_available_events[id].notify(sc_core::SC_ZERO_TIME);
    }
    tlm_generic_payload_rpc b_transport_rpc(int id, tlm_generic_payload_rpc t)
    {
        tlm::tlm_generic_payload trans;
        t.deep_copy_to_tlm(trans);
        //       SCP_DEBUG(()) << name() << " deep copy to_tlm " << txn_str(trans);
        sc_core::sc_time delay = sc_core::sc_time(t.m_quantum_time, sc_core::SC_SEC);
        sc_core::sc_time other_time = sc_core::sc_time(t.m_sc_time, sc_core::SC_SEC);

        //       SCP_DEBUG(()) << "Here"<<m_qk->time_to_sync();
        //        m_qk->sync();
        //        SCP_DEBUG(()) << "THere";
        //        SCP_DEBUG(()) << getpid() <<" IS THE rpc RPC PID " <<
        //        std::this_thread::get_id() <<" is the thread ID";
        m_sc.run_on_sysc([&] {
            //             SCP_DEBUG(()) <<"gere "<<id;
            //        SCP_DEBUG(()) << getpid() <<" IS THE rpc RPC PID " <<
            //        std::this_thread::get_id()
            //        <<" is the thread ID";
            //             m_qk->set(other_time+delay - sc_core::sc_time_stamp());
            //             SCP_DEBUG(()) << "WHAT Sync";
            //             m_qk->sync();
            //             m_qk->reset();
            initiator_sockets[id]->b_transport(trans, delay);
            //             SCP_DEBUG(()) << "WHAT";
            //             m_qk->sync();
        });
        t.from_tlm(trans);
        t.m_quantum_time = delay.to_seconds();
        //        SCP_DEBUG(()) << name() << " from_tlm " << txn_str(trans) ;
        return t;
    }

    /* Debug transport interface */
    unsigned int transport_dbg(int id, tlm::tlm_generic_payload& trans)
    {
        if (is_local_mode()) {
            return m_container->fw_transport_dbg(id, trans);
        }
        SCP_DEBUG(()) << name() << " ->remote debug tlm " << txn_str(trans);
        tlm_generic_payload_rpc t;
        tlm_generic_payload_rpc r;

        t.from_tlm(trans);
        r = do_rpc_as<tlm_generic_payload_rpc>(do_rpc_call("dbg_tspt", id, t));
        r.update_to_tlm(trans);
        SCP_DEBUG(()) << name() << " <-remote debug tlm done " << txn_str(trans);
        // this is not entirely accurate, but see below
        return trans.get_response_status() == tlm::TLM_OK_RESPONSE ? trans.get_data_length() : 0;
    }
    tlm_generic_payload_rpc transport_dbg_rpc(int id, tlm_generic_payload_rpc t)
    {
        tlm::tlm_generic_payload trans;
        t.deep_copy_to_tlm(trans);
        int ret_len;
        SCP_DEBUG(()) << name() << " remote-> debug tlm " << txn_str(trans);
        //        m_sc.run_on_sysc([&] {
        ret_len = initiator_sockets[id]->transport_dbg(trans);
        //            });
        t.from_tlm(trans);
        SCP_DEBUG(()) << name() << " remote<- debug tlm done " << txn_str(trans);

        if (!(trans.get_data_length() == ret_len || trans.get_response_status() != tlm::TLM_OK_RESPONSE)) {
            assert(false);
            SCP_WARN(()) << "debug transaction not able to access required length of data.";
        }
        return t;
    }

    bool get_direct_mem_ptr(int id, tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data)
    {
        if (is_local_mode()) {
            return m_container->fw_get_direct_mem_ptr(id, trans, dmi_data);
        }
        tlm::tlm_dmi* c;
        SCP_DEBUG(()) << " " << name() << " get_direct_mem_ptr to address "
                      << "0x" << std::hex << trans.get_address();

#ifdef DMICACHE
        c = in_cache(trans.get_address());
        if (c) {
            //            SCP_DEBUG(()) << "In Cache " << std::hex << c->get_start_address() <<
            //            "
            //            - " << std::hex << c->get_end_address() ;
            dmi_data = *c;
            return !(dmi_data.is_none_allowed());
        }
#endif
        tlm_generic_payload_rpc t;
        tlm_dmi_rpc r;
        t.from_tlm(trans);
        //        SCP_DEBUG(()) << name() << " DMI socket ID " << id << " From_tlm " <<
        //        txn_str(trans)
        //        ;
        r = do_rpc_as<tlm_dmi_rpc>(do_rpc_call("dmi_req", id, t));

        if (r.m_shmem_size == 0) {
            SCP_DEBUG(()) << name() << "DMI OK, but no shared memory available?" << trans.get_address();
            return false;
        }
        //        SCP_DEBUG(()) << "Got " << std::hex << r.m_dmi_start_address << " - " <<
        //        std::hex << r.m_dmi_end_address ; c = in_cache(r.m_shmem_fn); if (c) {
        //            dmi_data = *c;
        //        } else {
        r.to_tlm(dmi_data);
//            SCP_DEBUG(()) << "Adding " << r.m_shmem_fn << " " << dmi_data.get_start_address()
//                      << " to cache";
#ifdef DMICACHE
        assert(m_dmi_cache.count(dmi_data.get_start_address()) == 0);
        m_dmi_cache[dmi_data.get_start_address()] = dmi_data;
#endif
        //        }
        //        SCP_DEBUG(()) << name() << "DMI to " <<trans.get_address()<<" status "
        //        <<!(dmi_data.is_none_allowed()) <<" range " << std::hex <<
        //        dmi_data.get_start_address() << " - " << std::hex <<
        //        dmi_data.get_end_address() <<
        //        "";
        return !(dmi_data.is_none_allowed());
    }

    tlm_dmi_rpc get_direct_mem_ptr_rpc(int id, tlm_generic_payload_rpc t)
    {
        tlm::tlm_generic_payload trans;
        t.deep_copy_to_tlm(trans);

        SCP_DEBUG(()) << " " << name() << " get_direct_mem_ptr " << txn_str(trans);

        tlm::tlm_dmi dmi_data;
        tlm_dmi_rpc ret;
        ret.m_shmem_size = 0;
        if (initiator_sockets[id]->get_direct_mem_ptr(trans, dmi_data)) {
            ShmemIDExtension* ext = trans.get_extension<ShmemIDExtension>();
            if (!ext) return ret;
            ret.from_tlm(dmi_data, ext);
        }
        return ret;
    }

    /* Invalidate DMI Interface */
    void invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end)
    {
        if (is_local_mode()) {
            m_container->fw_invalidate_direct_mem_ptr(start, end);
        }
        SCP_DEBUG(()) << " " << name() << " invalidate_direct_mem_ptr "
                      << " start address 0x" << std::hex << start << " end address 0x" << std::hex << end;
        do_rpc_async_call("dmi_inv", start, end);
    }
    void invalidate_direct_mem_ptr_rpc(sc_dt::uint64 start, sc_dt::uint64 end)
    {
        SCP_DEBUG(()) << " " << name() << " invalidate_direct_mem_ptr "
                      << " start address 0x" << std::hex << start << " end address 0x" << std::hex << end;
#ifdef DMICACHE
        cache_clean(start, end);
#endif
        for (int i = 0; i < target_sockets.size(); i++) {
            target_sockets[i]->invalidate_direct_mem_ptr(start, end);
        }
    }

    /**
     * find if the fully qualified parameter name belongs to a set of strings
     */
    bool is_self_param(const std::string& parent, const std::string& parname, const std::string& value)
    {
        std::vector<std::string> match_words = {
            "args", "moduletype", "initiator_socket", "target_socket", "initiator_signal_socket", "target_signal_socket"
        };
        return (std::find_if(match_words.begin(), match_words.end(), [parent, parname, value](std::string entry) {
                    std::string search_str = parent + "." + entry;
                    if (entry == "moduletype" && value == "\"RemotePass\"") search_str = entry;
                    return parname.find(search_str) != std::string::npos;
                }) != match_words.end());
    }

    str_pairs get_cci_db()
    {
        std::string parent = std::string(name());
        str_pairs ret;
        for (auto p : m_broker.get_unconsumed_preset_values()) {
            std::string name = p.first;
            std::string k = p.first;
            if (k.find(parent) == 0) {
                if (is_self_param(parent, k, p.second.to_json())) continue;
                k = k.substr(parent.length() + 1);
                ret.push_back(std::make_pair("$" + k, p.second.to_json()));
                // mark parameters in the remote as 'used'
                m_broker.ignore_unconsumed_preset_values(
                    [name](const std::pair<std::string, cci::cci_value>& iv) -> bool { return iv.first == name; });
            } else if (k.find('.') == std::string::npos) {
                ret.push_back(std::make_pair(k, p.second.to_json()));
            }
        }
        return ret;
    }

    void set_cci_db(str_pairs db)
    {
        std::string modname = std::string(name());
        std::string parent = std::string(modname).substr(0, modname.find_last_of("."));

        for (auto p : db) {
            std::string parname;
            if (p.first[0] == '$') {
                parname = parent + "." + p.first.substr(1);
            } else if (p.first[0] == '@') {
                parname = modname + "." + p.first.substr(1);
            } else {
                parname = p.first;
            }
            auto handle = m_broker.get_param_handle(parname);
            if (handle.is_valid())
                handle.set_cci_value(cci::cci_value::from_json(p.second));
            else
                m_broker.set_preset_cci_value(parname, cci::cci_value::from_json(p.second));

            SCP_DEBUG(()) << "Setting " << parname << " to " << p.second;
        }
    }

    std::vector<std::string> get_extra_argv()
    {
        std::vector<std::string> argv;
        std::list<std::string> argv_cci_children = gs::sc_cci_children((std::string(name()) + ".remote_argv").c_str());
        if (!argv_cci_children.empty())
            std::transform(argv_cci_children.begin(), argv_cci_children.end(), std::back_inserter(argv),
                           [this](const std::string& arg) {
                               std::string arg_full_name = (std::string(name()) + ".remote_argv." + arg);
                               return gs::cci_get<std::string>(cci::cci_get_broker(), arg_full_name);
                           });
        return argv;
    }

public:
    PassRPC(const sc_core::sc_module_name& nm, bool is_local = false)
        : sc_core::sc_module(nm)
        , m_broker(cci::cci_get_broker())
        , initiator_sockets("initiator_socket")
        , target_sockets("target_socket")
        , initiator_signal_sockets("initiator_signal_socket")
        , target_signal_sockets("target_signal_socket")
        , p_cport("client_port", 0,
                  "The port that should be used to connect this client to the "
                  "remote server")
        , p_sport("server_port", 0, "The port that should be used to server on")
        , p_exec_path("exec_path", "",
                      "The path to the executable that should be started by "
                      "the bridge")
        , m_is_local(is_local)
        , p_sync_policy("sync_policy", "multithread-unconstrained", "Sync policy for the remote")
        , p_tlm_initiator_ports_num("tlm_initiator_ports_num", 0, "number of tlm initiator ports")
        , p_tlm_target_ports_num("tlm_target_ports_num", 0, "number of tlm target ports")
        , p_initiator_signals_num("initiator_signals_num", 0, "number of initiator signals")
        , p_target_signals_num("target_signals_num", 0, "number of target signals")
        , cancel_waiting(false)
    {
        SigHandler::get().add_sig_handler(SIGINT, SigHandler::Handler_CB::PASS);
        SigHandler::get().register_on_exit_cb([this]() { stop(); });
        sc_tid = std::this_thread::get_id();
        SCP_DEBUG(()) << "PassRPC constructor";
        m_container = dynamic_cast<gs::ModuleFactory::ContainerBase*>(get_parent_object());
        if (is_local_mode()) {
            SCP_DEBUG(()) << "Working in LOCAL mode!";
        } else {
            SCP_DEBUG(()) << getpid() << " IS THE RPC PID " << std::this_thread::get_id() << " is the thread ID";
            // always serve on a new port.
            server = new rpc::server(p_sport);
            server->suppress_exceptions(true);
            p_sport = server->port();
            assert(p_sport > 0);

            if (p_cport.get_value() == 0 &&
                getenv((std::string(GS_Process_Server_Port) + std::to_string(getpid())).c_str())) {
                p_cport = std::stoi(
                    std::string(getenv((std::string(GS_Process_Server_Port) + std::to_string(getpid())).c_str())));
            }

            // other end contacted us, connect to their port
            // and return back the cci database
            server->bind("reg", [&](int port) {
                SCP_DEBUG(()) << "reg " << name() << " pid: " << getpid();
                assert(p_cport == 0 && client == nullptr);
                p_cport = port;
                if (!client) client = new rpc::client("localhost", p_cport);
                std::unique_lock<std::mutex> ul(client_conncted_mut);
                is_client_connected.notify_one();
                ul.unlock();
                // we are not interested in the return future from async_call
                do_rpc_async_call("sock_pair", pahandler.get_sockpair_fd0(), pahandler.get_sockpair_fd1());
                return get_cci_db();
            });

            // would it be better to have a 'remote cci broker' that connected back,

            server->bind("cci_db", [&](str_pairs db) {
                if (sc_core::sc_get_status() > sc_core::sc_status::SC_BEFORE_END_OF_ELABORATION) {
                    std::cerr << "Attempt to do cci_db() RPC after "
                                 "sc_core::sc_status::SC_BEFORE_END_OF_ELABORATION"
                              << std::endl;
                    exit(1);
                }
                std::lock_guard<std::mutex> lg(m_cci_db_mut);
                m_cci_db.insert(m_cci_db.end(), db.begin(), db.end());
                return;
            });

            server->bind("status", [&](int s) {
                SCP_DEBUG(()) << "SIMULATION STATE " << name() << " to status " << s;
                assert(s > m_remote_status);
                m_remote_status = static_cast<sc_core::sc_status>(s);
                std::lock_guard<std::mutex> lg(sc_status_mut);
                is_sc_status_set.notify_one();
                return;
            });

            server->bind("b_tspt", [&](int id, tlm_generic_payload_rpc txn) {
                try {
                    return PassRPC::b_transport_rpc(id, txn);
                } catch (std::runtime_error const& e) {
                    std::cerr << "Main Error: '" << e.what() << "'\n";
                    exit(1);
                } catch (const std::exception& exc) {
                    std::cerr << "main Error: '" << exc.what() << "'\n";
                    exit(1);
                } catch (...) {
                    std::cerr << "Unknown error (main.cc)!\n";
                    exit(1);
                }
            });

            server->bind("dbg_tspt", [&](int id, tlm_generic_payload_rpc txn) {
                SCP_DEBUG(()) << "Got DBG Tspt";
                return PassRPC::transport_dbg_rpc(id, txn);
            });

            server->bind("dmi_inv", [&](uint64_t start, uint64_t end) {
                return PassRPC::invalidate_direct_mem_ptr_rpc(start, end);
            });

            server->bind("dmi_req",
                         [&](int id, tlm_generic_payload_rpc txn) { return PassRPC::get_direct_mem_ptr_rpc(id, txn); });

            server->bind("exit", [&](int i) {
                SCP_DEBUG(()) << "exit " << name();
                m_sc.run_on_sysc([&] {
                    rpc::this_session().post_exit();
                    delete client;
                    client = nullptr;
                    sc_core::sc_stop();
                });
                // m_qk->stop();
                return;
            });

            server->bind("signal", [&](int i, bool v) {
                if (sc_core::sc_get_status() < sc_core::sc_status::SC_START_OF_SIMULATION) {
                    std::lock_guard<std::mutex> lg(sig_queue_mut);
                    sig_queue.push(std::make_pair(i, v));
                    return;
                }
                m_sc.run_on_sysc([&] { initiator_signal_sockets[i]->write(v); },
                                 (sc_core::sc_get_status() < sc_core::sc_status::SC_RUNNING ? false : true));
                return;
            });

            server->bind("sock_pair", [&](int sock_fd0, int sock_fd1) {
                pahandler.recv_sockpair_fds_from_remote(sock_fd0, sock_fd1);
                pahandler.check_parent_conn_nth([&]() {
                    std::cerr << "remote process (" << getpid() << ") detected parent (" << pahandler.get_ppid()
                              << ") exit!" << std::endl;
                });
                return;
            });

            // m_qk = tlm_quantumkeeper_factory(p_sync_policy);
            // m_qk->reset();
            server->async_run(1);

            if (p_cport) {
                SCP_INFO(()) << "Connecting client on port " << p_cport;
                if (!client) client = new rpc::client("localhost", p_cport);
                set_cci_db(do_rpc_as<str_pairs>(do_rpc_call("reg", (int)p_sport)));
            }
        }

        btspt_waiter = std::make_unique<trans_waiter>("btspt_waiter", p_tlm_target_ports_num.get_value());

        initiator_sockets.init(p_tlm_initiator_ports_num.get_value(), [this](const char* n, int i) {
            return new initiator_socket_spying(n, [&](std::string s) -> void { remote_register_boundto(s); });
        });
        target_sockets.init(p_tlm_target_ports_num.get_value(),
                            [this](const char* n, int i) { return new tlm_target_socket(n); });
        initiator_signal_sockets.init(p_initiator_signals_num.get_value(),
                                      [this](const char* n, int i) { return new InitiatorSignalSocket<bool>(n); });
        target_signal_sockets.init(p_target_signals_num.get_value(),
                                   [this](const char* n, int i) { return new TargetSignalSocket<bool>(n); });

        for (int i = 0; i < p_tlm_target_ports_num.get_value(); i++) {
            target_sockets[i].register_b_transport(this, &PassRPC::b_transport, i);
            target_sockets[i].register_transport_dbg(this, &PassRPC::transport_dbg, i);
            target_sockets[i].register_get_direct_mem_ptr(this, &PassRPC::get_direct_mem_ptr, i);
        }

        for (int i = 0; i < p_tlm_initiator_ports_num.get_value(); i++) {
            initiator_sockets[i].register_invalidate_direct_mem_ptr(this, &PassRPC::invalidate_direct_mem_ptr);
        }

        for (int i = 0; i < p_target_signals_num.get_value(); i++) {
            target_signal_sockets[i].register_value_changed_cb([&, i](bool value) {
                if (is_local_mode()) {
                    m_container->fw_handle_signal(i, value);
                    return;
                }
                do_rpc_async_call("signal", i, value);
            });
        }

        if (!is_local_mode()) {
            if (!p_exec_path.get_value().empty()) {
                SCP_INFO(()) << "Forking remote " << p_exec_path.get_value();
                pahandler.init_peer_conn_checker();

                std::vector<std::string> extra_args;
                extra_args = get_extra_argv();
                m_remote_args.reserve(extra_args.size() + 2);
                m_remote_args.push_back(p_exec_path.get_value().c_str());
                std::transform(extra_args.begin(), extra_args.end(), std::back_inserter(m_remote_args),
                               [](const std::string& s) { return s.c_str(); });
                m_remote_args.push_back(0);
                char val[DECIMAL_PORT_NUM_STR_LEN + 1]; // can't be bigger than this.
                snprintf(val, DECIMAL_PORT_NUM_STR_LEN + 1, "%d", p_sport.get_value());
                m_child_pid = fork();
                if (m_child_pid > 0) {
                    pahandler.setup_parent_conn_checker();
                    SigHandler::get().set_nosig_chld_stop();
                    SigHandler::get().add_sig_handler(SIGCHLD, SigHandler::Handler_CB::EXIT);
                } else if (m_child_pid == 0) {
                    char key[GS_Process_Server_Port_Len + DECIMAL_PID_T_STR_LEN + 1]; // can't be bigger than this.
                    snprintf(key, GS_Process_Server_Port_Len + DECIMAL_PID_T_STR_LEN + 1, "%s%d",
                             GS_Process_Server_Port, getpid());
                    setenv(key, val, 1);

                    execv(p_exec_path.get_value().c_str(), const_cast<char**>(&m_remote_args[0]));

                    SCP_FATAL(()) << "Unable to exec the remote child process, '" << p_exec_path.get_value()
                                  << "', error: " << std::strerror(errno);
                } else {
                    SCP_FATAL(()) << "failed to fork remote process, error: " << std::strerror(errno);
                }
            }

            // Make sure by now the client is connected so we can send/recieve.
            std::unique_lock<std::mutex> ul(client_conncted_mut);
            is_client_connected.wait(ul, [&]() { return (p_cport > 0 || cancel_waiting); });
            ul.unlock();
            send_status();
        }
    }                                                                              // namespace gs
    PassRPC(const sc_core::sc_module_name& nm, int port): PassRPC(nm, "", port){}; // convenience constructor

    void send_status()
    {
        SCP_DEBUG(()) << "SIMULATION STATE send " << name() << " to status " << sc_core::sc_get_status();
        do_rpc_call("status", static_cast<int>(sc_core::sc_get_status()));
        std::unique_lock<std::mutex> ul(sc_status_mut);
        is_sc_status_set.wait(ul, [&]() { return (m_remote_status >= sc_core::sc_get_status() || cancel_waiting); });
        ul.unlock();
        SCP_DEBUG(()) << "SIMULATION STATE synced " << name() << " to status " << sc_core::sc_get_status();
    }

    void handle_before_sim_start_signals()
    {
        std::lock_guard<std::mutex> lg(sig_queue_mut);
        while (!sig_queue.empty()) {
            std::pair<int, bool> sig = sig_queue.front();
            sig_queue.pop();
            m_sc.run_on_sysc([&] { initiator_signal_sockets[sig.first]->write(sig.second); }, false);
        }
    }

    void stop()
    {
        {
            std::lock_guard<std::mutex> lg(stop_mutex);
            if (cancel_waiting || is_local_mode()) return;
            cancel_waiting = true;
        }
        {
            std::lock_guard<std::mutex> cc_lg(client_conncted_mut);
            is_client_connected.notify_one();
        }
        {
            std::lock_guard<std::mutex> scs_lg(sc_status_mut);
            is_sc_status_set.notify_one();
        }
        btspt_waiter->stop();
        if (server) {
            server->close_sessions();
            server->stop();
            delete server;
            server = nullptr;
        }
        if (client) {
            do_rpc_async_call("exit", 0);
            delete client;
            client = nullptr;
        }
    }

    void stop_and_exit()
    {
        stop();
        _Exit(EXIT_SUCCESS);
    }

    void before_end_of_elaboration() override
    {
        if (is_local_mode()) return;
        send_status();
        std::lock_guard<std::mutex> lg(m_cci_db_mut);
        set_cci_db(m_cci_db);
    }

    void end_of_elaboration() override
    {
        if (is_local_mode()) return;
        send_status();
    }

    void start_of_simulation() override
    {
        if (is_local_mode()) return;
        send_status();
        handle_before_sim_start_signals();
        // m_qk->start();
    }

    PassRPC() = delete;
    PassRPC(const PassRPC&) = delete;
    ~PassRPC()
    {
        if (is_local_mode()) return;
        // m_qk->stop();
        SCP_DEBUG(()) << "EXIT " << name();
        stop();
#ifdef DMICACHE
        m_dmi_cache.clear();
#endif
    }

    void end_of_simulation() override
    {
        if (is_local_mode()) return;
        // m_qk->stop();
        stop();
    }
}; // namespace gs
template <unsigned int BUSWIDTH = DEFAULT_TLM_BUSWIDTH>
class LocalPass : public PassRPC<>
{
public:
    SCP_LOGGER(());
    LocalPass(const sc_core::sc_module_name& n): PassRPC(n, true) { SCP_DEBUG(()) << "LocalPass constructor"; }

    virtual ~LocalPass() = default;
};

template <unsigned int BUSWIDTH = DEFAULT_TLM_BUSWIDTH>
class RemotePass : public PassRPC<>
{
public:
    SCP_LOGGER(());
    RemotePass(const sc_core::sc_module_name& n): PassRPC(n, false) { SCP_DEBUG(()) << "RemotePass constructor"; }

    virtual ~RemotePass() = default;
};

} // namespace gs
// need to be register without the dynamic library feature because of the order of execution of the code.
typedef gs::LocalPass<> LocalPass;
typedef gs::RemotePass<> RemotePass;
GSC_MODULE_REGISTER(LocalPass);
GSC_MODULE_REGISTER(RemotePass);

#endif

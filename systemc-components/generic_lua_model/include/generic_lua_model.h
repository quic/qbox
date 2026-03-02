/*
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GREENSOCS_BASE_COMPONENTS_GENERIC_LUA_MODEL_H
#define GREENSOCS_BASE_COMPONENTS_GENERIC_LUA_MODEL_H

#include <systemc>
#include <tlm>
#include <tlm_utils/peq_with_get.h>
#include <cci_configuration>
#include <cciutils.h>
#include <scp/report.h>
#include <module_factory_registery.h>
#include <ports/target-signal-socket.h>
#include <ports/initiator-signal-socket.h>
#include <gs_memory.h>
#include <reg_router.h>
#include <registers.h>
#include <vector>
#include <tuple>
#include <memory>
#include <functional>
#include <algorithm>
#include <unordered_map>
#include <list>
#include <tlm_sockets_buswidth.h>

#define DEFAULT_REG_MASK 0xFFFFFFFFUL

namespace gs {

struct generic_lua_model_signal_payload {
    std::shared_ptr<std::pair<uint32_t, bool>> sig_num_val_pair;
    std::string sig_name;
    generic_lua_model_signal_payload(uint32_t sig_num, bool val, const std::string& _name = "");
};

struct generic_lua_model_txn_payload {
    std::shared_ptr<tlm::tlm_generic_payload> txn;
    std::array<unsigned char, sizeof(uint32_t)> buffer;
    generic_lua_model_txn_payload(uint64_t addr, uint32_t value);
};

class generic_lua_model : public sc_core::sc_module
{
protected:
    SCP_LOGGER();

    using tlm_initiator_socket_t = tlm_utils::simple_initiator_socket_b<
        generic_lua_model, DEFAULT_TLM_BUSWIDTH, tlm::tlm_base_protocol_types, sc_core::SC_ZERO_OR_MORE_BOUND>;
    using tlm_target_socket_t = tlm_utils::simple_target_socket_b<
        generic_lua_model, DEFAULT_TLM_BUSWIDTH, tlm::tlm_base_protocol_types, sc_core::SC_ZERO_OR_MORE_BOUND>;
    using reg_ptr_t = std::shared_ptr<gs::gs_register<uint32_t>>;
    using data_t = std::vector<std::tuple<sc_core::sc_time, uint64_t, uint32_t>>;
    using signals_t = std::vector<std::tuple<sc_core::sc_time, uint32_t, bool>>;
    using reactions_t = std::pair<data_t, signals_t>;

    enum class action_source { REGISTER_CB, TARGET_SIGNAL, START_OF_SIM, UNKNOWN };
    enum class actions { PRE_READ, PRE_WRITE, POST_READ, POST_WRITE, UNKNOWN };
    struct enum_hash {
        template <typename T>
        std::size_t operator()(T inst) const
        {
            return static_cast<std::size_t>(inst);
        }
    };

    struct register_info_t {
        std::string reg_name;
        actions action;
        uint64_t address;
        uint32_t reg_mask;
        uint32_t default_value;
        bool is_default_value_set;
        bool value_set;
        uint32_t value;
        uint32_t mask;
        bool if_val_set;
        uint32_t if_val;
        data_t data;
        signals_t signals;
        reg_ptr_t reg_ptr;

        register_info_t()
            : reg_name("")
            , action(actions::UNKNOWN)
            , address(0xffffffffffffffffULL)
            , reg_mask(DEFAULT_REG_MASK)
            , default_value(0)
            , value_set(false)
            , value(0)
            , mask(DEFAULT_REG_MASK)
            , if_val_set(false)
            , if_val(0)
            , data{}
            , signals{}
            , reg_ptr(nullptr)
        {
        }
        ~register_info_t() = default;
    };

public:
    generic_lua_model(sc_core::sc_module_name _name, sc_core::sc_object* _container, sc_core::sc_object* _reg_memory);

    generic_lua_model(sc_core::sc_module_name _name, gs::reg_router<>* _reg_router, gs::gs_memory<>* _reg_memory);

    ~generic_lua_model() = default;

protected:
    void set_base_address();
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int transport_dbg(tlm::tlm_generic_payload& trans);
    bool get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data);
    void invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end);
    void before_end_of_elaboration() override;
    void start_of_simulation() override;
    void dispatch(const std::string& _name);
    void set_data_and_signals_actions_from_cci(const std::string& cci_name, data_t& data, signals_t& signals);
    void set_reg_mask_and_value_actions_from_cci(const std::string& cci_name, register_info_t& regs_info);
    void dispatch_signals_actions(const std::string& cci_name, uint32_t sig_num, bool sig_val);
    void dispatch_registers_cb_actions(const std::string& cci_name, const std::string& reg_name, uint64_t address,
                                       uint32_t reg_mask, bool is_default_value_set, uint32_t default_value,
                                       actions action);
    void bind_register(register_info_t& regs_info);
    void register_input_signal(uint32_t sig_num, bool value, const data_t& data, const signals_t& signals);
    void reg_action_cb(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay, const register_info_t& regs_info);
    void target_signal_cb(int id, bool value);
    void trigger_signals(const signals_t& signals, action_source act_src);
    void write_data(const data_t& data, action_source act_src);
    void exec_data_thread(action_source act_src);
    void exec_sig_thread(action_source act_src);
    void reg_data_thread();
    void reg_signals_thread();
    void sos_data_thread();
    void sos_signals_thread();
    void target_sig_data_thread();
    void target_sig_signals_thread();

    template <class T>
    void cci_set(std::string n, T value)
    {
        auto handle = m_broker.get_param_handle(n);
        if (handle.is_valid())
            handle.set_cci_value(cci::cci_value(value));
        else if (!m_broker.has_preset_value(n)) {
            m_broker.set_preset_cci_value(n, cci::cci_value(value));
        } else {
            SCP_FATAL(())("Trying to re-set a value? {}", n);
        }
    }

public:
    tlm_initiator_socket_t initiator_socket;
    tlm_target_socket_t target_socket;
    sc_core::sc_vector<TargetSignalSocket<bool>> target_signal_sockets;
    sc_core::sc_vector<InitiatorSignalSocket<bool>> initiator_signal_sockets;
    TargetSignalSocket<bool> reset;

private:
    tlm_initiator_socket_t m_initiator_socket;
    cci::cci_broker_handle m_broker;
    std::unordered_map<action_source, std::unique_ptr<tlm_utils::peq_with_get<tlm::tlm_generic_payload>>, enum_hash>
        m_data_peq;
    std::unordered_map<action_source, std::list<generic_lua_model_txn_payload>, enum_hash> m_data_pending_list;
    std::unordered_map<action_source, std::unique_ptr<tlm_utils::peq_with_get<std::pair<uint32_t, bool>>>, enum_hash>
        m_signals_peq;
    std::unordered_map<action_source, std::list<generic_lua_model_signal_payload>, enum_hash> m_signals_pending_list;
    gs::reg_router<>* m_reg_router;
    gs::gs_memory<>* m_reg_memory;
    std::vector<register_info_t> m_registers;
    std::unordered_map<uint32_t, std::unordered_map<bool, reactions_t>> m_signals;
    std::vector<reactions_t> m_start_of_sim_ts_ns;
    uint64_t m_base_addr;
};
} // namespace gs

extern "C" void module_register();

#endif

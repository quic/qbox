/*
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "generic_lua_model.h"

namespace gs {

generic_lua_model_signal_payload::generic_lua_model_signal_payload(uint32_t sig_num, bool val, const std::string& _name)
    : sig_name(_name)
{
    sig_num_val_pair = std::make_shared<std::pair<uint32_t, bool>>(std::make_pair(sig_num, val));
}

generic_lua_model_txn_payload::generic_lua_model_txn_payload(uint64_t addr, uint32_t value)
{
    txn = std::make_shared<tlm::tlm_generic_payload>();
    std::memcpy(buffer.data(), &value, sizeof(uint32_t));
    txn->set_address(addr);
    txn->set_data_length(sizeof(uint32_t));
    txn->set_streaming_width(sizeof(uint32_t));
    txn->set_data_ptr(buffer.data());
    txn->set_write();
}

/**
 * This constructor signature is used when a container object is already instanciated and has a
 * sub module called reg_router.
 */
generic_lua_model::generic_lua_model(sc_core::sc_module_name _name, sc_core::sc_object* _container,
                                     sc_core::sc_object* _reg_memory)
    : generic_lua_model(
          _name,
          dynamic_cast<gs::reg_router<>*>(gs::find_sc_obj(_container, std::string(_container->name()) + ".reg_router")),
          dynamic_cast<gs::gs_memory<>*>(_reg_memory))
{
}

generic_lua_model::generic_lua_model(sc_core::sc_module_name _name, gs::reg_router<>* _reg_router,
                                     gs::gs_memory<>* _reg_memory)
    : sc_core::sc_module(_name)
    , initiator_socket("initiator_socket")
    , target_socket("target_socket")
    , target_signal_sockets("target_signal_socket")
    , initiator_signal_sockets("initiator_signal_socket")
    , m_broker(cci::cci_get_broker())
    , m_reg_router(_reg_router)
    , m_reg_memory(_reg_memory)
    , reset("reset")
    , m_base_addr(0)
{
    SCP_TRACE(())("Constructor");
    if (!m_reg_router) {
        SCP_FATAL(()) << "reg_router is NULL!";
    }
    if (!m_reg_memory) {
        SCP_FATAL(()) << "reg_memory is NULL!";
    }
    set_base_address();
    m_initiator_socket.bind(m_reg_router->target_socket);
    target_socket.register_b_transport(this, &generic_lua_model::b_transport);
    target_socket.register_transport_dbg(this, &generic_lua_model::transport_dbg);
    target_socket.register_get_direct_mem_ptr(this, &generic_lua_model::get_direct_mem_ptr);
    initiator_socket.register_invalidate_direct_mem_ptr(this, &generic_lua_model::invalidate_direct_mem_ptr);

    dispatch(std::string(this->name()));

    for (uint32_t i = 0; i < target_signal_sockets.size(); i++) {
        target_signal_sockets[i].register_value_changed_cb([this, i](bool value) { target_signal_cb(i, value); });
    }

    m_data_peq[action_source::REGISTER_CB] = std::make_unique<tlm_utils::peq_with_get<tlm::tlm_generic_payload>>(
        "reg_cb_data_peq");
    m_data_peq[action_source::TARGET_SIGNAL] = std::make_unique<tlm_utils::peq_with_get<tlm::tlm_generic_payload>>(
        "target_signal_data_peq");
    m_data_peq[action_source::START_OF_SIM] = std::make_unique<tlm_utils::peq_with_get<tlm::tlm_generic_payload>>(
        "start_of_sim_data_peq");
    m_signals_peq[action_source::REGISTER_CB] = std::make_unique<tlm_utils::peq_with_get<std::pair<uint32_t, bool>>>(
        "reg_cb_signals_peq");
    m_signals_peq[action_source::TARGET_SIGNAL] = std::make_unique<tlm_utils::peq_with_get<std::pair<uint32_t, bool>>>(
        "target_signal_signals_peq");
    m_signals_peq[action_source::START_OF_SIM] = std::make_unique<tlm_utils::peq_with_get<std::pair<uint32_t, bool>>>(
        "start_of_sim_signals_peq");

    SC_THREAD(reg_data_thread);
    SC_THREAD(reg_signals_thread);
    SC_THREAD(sos_data_thread);
    SC_THREAD(sos_signals_thread);
    SC_THREAD(target_sig_data_thread);
    SC_THREAD(target_sig_signals_thread);

    reset.register_value_changed_cb([&](bool value) {
        if (value) {
            SCP_WARN(()) << "Reset";
            for (auto& d_pair : m_data_peq) {
                d_pair.second->cancel_all();
            }
            for (auto& dp_pair : m_data_pending_list) {
                dp_pair.second.clear();
            }
            for (auto& s_pair : m_signals_peq) {
                s_pair.second->cancel_all();
            }
            for (auto& sp_pair : m_signals_pending_list) {
                sp_pair.second.clear();
            }
            start_of_simulation();
        }
    });
}

void generic_lua_model::set_base_address()
{
    bool relative_addr;
    uint64_t base_addr;
    if (!gs::cci_get<uint64_t>(m_broker, std::string(this->name()) + ".target_socket.address", base_addr)) {
        SCP_TRACE(()) << "can't get address CCI parameter for target_socket. either target_socket is not used or "
                         "address CCI parameter is not set!";
        return;
    }
    if (!gs::cci_get<bool>(m_broker, std::string(this->name()) + ".target_socket.relative_addresses", relative_addr)) {
        relative_addr = true;
    }
    if (relative_addr) {
        m_base_addr = base_addr;
    }
}

void generic_lua_model::b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
{
    /**
     * Forward b_transport
     */
    trans.set_address(trans.get_address() + m_base_addr);
    m_initiator_socket->b_transport(trans, delay);
}

unsigned int generic_lua_model::transport_dbg(tlm::tlm_generic_payload& trans)
{
    /**
     * Forward transport_dbg
     */
    trans.set_address(trans.get_address() + m_base_addr);
    return m_initiator_socket->transport_dbg(trans);
}

bool generic_lua_model::get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data)
{
    /**
     * DMI is not allowed
     */
    return false;
}

void generic_lua_model::invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end)
{
    /**
     * DMI is not allowed
     */
    return;
}

void generic_lua_model::dispatch(const std::string& _name)
{
    if (_name.empty()) {
        SCP_FATAL(()) << "dispatch: dispatch function was called with empty string!";
    }
    data_t sos_data;
    signals_t sos_signals;
    /**
     *   This will handle the start of simulation after_ns entries, i.e. {after_ns = 100, address = 0x123456, data =
     * 0x789abc}
     */
    set_data_and_signals_actions_from_cci(_name, sos_data, sos_signals);
    m_start_of_sim_ts_ns.push_back(std::make_pair(sos_data, sos_signals));

    auto children = gs::sc_cci_children(this->name());
    for (std::string s : children) {
        if (s.rfind(".registers") == (s.size() - std::string(".registers").size())) {
            auto reg_children = gs::sc_cci_children(std::string(_name + ".registers").c_str());
            for (std::string rs : reg_children) {
                std::string reg_name;
                uint64_t address;
                uint32_t reg_mask;
                uint32_t default_value = 0;
                bool is_default_value_set = false;
                if (std::count_if(rs.begin(), rs.end(), [](unsigned char c) { return std::isdigit(c); })) {
                    std::string cci_registers_name = _name + ".registers." + rs;
                    if (!gs::cci_get<std::string>(m_broker, cci_registers_name + ".name", reg_name)) {
                        SCP_FATAL(()) << "dispatch: can't get register name!";
                    }
                    if (!gs::cci_get<uint64_t>(m_broker, cci_registers_name + ".address", address)) {
                        SCP_FATAL(()) << "dispatch: can't get register address!";
                    }
                    if (!gs::cci_get<uint32_t>(m_broker, cci_registers_name + ".reg_mask", reg_mask)) {
                        reg_mask = DEFAULT_REG_MASK;
                    }
                    if (gs::cci_get<uint32_t>(m_broker, cci_registers_name + ".default", default_value)) {
                        is_default_value_set = true;
                    }
                    if (!gs::sc_cci_children(std::string(cci_registers_name + ".on_pre_read").c_str()).empty()) {
                        dispatch_registers_cb_actions(cci_registers_name + ".on_pre_read", reg_name, address, reg_mask,
                                                      is_default_value_set, default_value, actions::PRE_READ);
                    }
                    if (!gs::sc_cci_children(std::string(cci_registers_name + ".on_pre_write").c_str()).empty()) {
                        dispatch_registers_cb_actions(cci_registers_name + ".on_pre_write", reg_name, address, reg_mask,
                                                      is_default_value_set, default_value, actions::PRE_WRITE);
                    }
                    if (!gs::sc_cci_children(std::string(cci_registers_name + ".on_post_read").c_str()).empty()) {
                        dispatch_registers_cb_actions(cci_registers_name + ".on_post_read", reg_name, address, reg_mask,
                                                      is_default_value_set, default_value, actions::POST_READ);
                    }
                    if (!gs::sc_cci_children(std::string(cci_registers_name + ".on_post_write").c_str()).empty()) {
                        dispatch_registers_cb_actions(cci_registers_name + ".on_post_write", reg_name, address,
                                                      reg_mask, is_default_value_set, default_value,
                                                      actions::POST_WRITE);
                    }
                }
            }
        } else if (s.rfind(".signals") == (s.size() - std::string(".signals").size())) {
            auto sig_children = gs::sc_cci_children(std::string(_name + ".signals").c_str());
            for (std::string ss : sig_children) {
                if (std::count_if(ss.begin(), ss.end(), [](unsigned char c) { return std::isdigit(c); })) {
                    std::string cci_signals_name = _name + ".signals." + ss;
                    std::string sig_name;
                    uint32_t sig_num;
                    if (!gs::cci_get<std::string>(m_broker, cci_signals_name + ".name", sig_name)) {
                        SCP_FATAL(()) << "dispatch: can't get signal name!";
                    } else {
                        target_signal_sockets.emplace_back_with_name(sig_name.c_str());
                        sig_num = target_signal_sockets.size() - 1;
                    }
                    if (!gs::sc_cci_children(std::string(cci_signals_name + ".on_true").c_str()).empty()) {
                        dispatch_signals_actions(cci_signals_name + ".on_true", sig_num, true);
                    }
                    if (!gs::sc_cci_children(std::string(cci_signals_name + ".on_false").c_str()).empty()) {
                        dispatch_signals_actions(cci_signals_name + ".on_false", sig_num, false);
                    }
                }
            }
        }
    }
}

void generic_lua_model::dispatch_signals_actions(const std::string& cci_name, uint32_t sig_num, bool sig_val)
{
    data_t data;
    signals_t signals;
    set_data_and_signals_actions_from_cci(cci_name, data, signals);
    register_input_signal(sig_num, sig_val, data, signals);
}

void generic_lua_model::dispatch_registers_cb_actions(const std::string& cci_name, const std::string& reg_name,
                                                      uint64_t address, uint32_t reg_mask, bool is_default_value_set,
                                                      uint32_t default_value, actions action)
{
    register_info_t regs_info;
    regs_info.reg_name = reg_name;
    regs_info.address = address + m_base_addr;
    regs_info.reg_mask = reg_mask;
    regs_info.default_value = default_value;
    regs_info.is_default_value_set = is_default_value_set;
    regs_info.action = action;
    set_reg_mask_and_value_actions_from_cci(cci_name, regs_info);
    set_data_and_signals_actions_from_cci(cci_name, regs_info.data, regs_info.signals);
    bind_register(regs_info);
}

void generic_lua_model::bind_register(register_info_t& regs_info)
{
    auto reg_it = std::find_if(m_registers.begin(), m_registers.end(), [&](const auto& info) {
        return info.reg_ptr && (info.reg_ptr->get_regname() == regs_info.reg_name);
    });
    bool reg_found = (reg_it != m_registers.end());
    reg_ptr_t reg;
    if (reg_found) {
        reg = reg_it->reg_ptr;
        if (reg->get_offset() != regs_info.address) {
            SCP_FATAL(()) << "Using the same register name: " << regs_info.reg_name
                          << " with different register address: 0x" << std::hex << regs_info.address
                          << " is not allowed!";
        }
    } else {
        reg = std::make_shared<gs::gs_register<uint32_t>>(regs_info.reg_name, regs_info.reg_name, regs_info.address, 1);
        reg->initiator_socket.bind(m_reg_memory->socket);
        std::string reg_target_socket_name = std::string(this->name()) + "." + reg->get_regname() + ".target_socket";
        std::string reg_address_str = reg_target_socket_name + ".address";
        std::string reg_size_str = reg_target_socket_name + ".size";
        cci_set<uint64_t>(reg_address_str, regs_info.address);
        cci_set<uint64_t>(reg_size_str, 1);
        m_reg_router->initiator_socket.bind(*reg);
        m_reg_router->rename_last(reg_target_socket_name);
    }
    regs_info.reg_ptr = reg;
    m_registers.push_back(regs_info);
}

void generic_lua_model::set_data_and_signals_actions_from_cci(const std::string& cci_name, data_t& data,
                                                              signals_t& signals)
{
    auto children = gs::sc_cci_children(cci_name.c_str());
    if (children.empty()) return;
    for (std::string s : children) {
        if (std::count_if(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); })) {
            double after_ns;
            uint64_t mem_addr;
            uint32_t mem_data;
            uint32_t sig_num;
            std::string sig_name;
            bool sig_value;

            if (!gs::cci_get<double>(m_broker, cci_name + "." + s + ".after_ns", after_ns)) {
                after_ns = 0;
            }
            if (gs::cci_get<uint64_t>(m_broker, cci_name + "." + s + ".address", mem_addr) &&
                gs::cci_get<uint32_t>(m_broker, cci_name + "." + s + ".data", mem_data)) {
                data.push_back(
                    std::make_tuple(sc_core::sc_time(after_ns, sc_core::sc_time_unit::SC_NS), mem_addr, mem_data));
            }
            if (gs::cci_get<std::string>(m_broker, cci_name + "." + s + ".name", sig_name) &&
                gs::cci_get<bool>(m_broker, cci_name + "." + s + ".output", sig_value)) {
                auto it = std::find_if(
                    initiator_signal_sockets.begin(), initiator_signal_sockets.end(),
                    [&sig_name](const InitiatorSignalSocket<bool>& sig) { return sig.basename() == sig_name; });
                if (it != initiator_signal_sockets.end()) {
                    sig_num = std::distance(initiator_signal_sockets.begin(), it);
                } else {
                    initiator_signal_sockets.emplace_back_with_name(sig_name.c_str());
                    sig_num = initiator_signal_sockets.size() - 1;
                }
                signals.push_back(
                    std::make_tuple(sc_core::sc_time(after_ns, sc_core::sc_time_unit::SC_NS), sig_num, sig_value));
            }
        }
    }
}
/**
 * implemented as a separate function, not part of set_data_and_signals_actions_from_cci(), as it should
 * only be used in the register callbacks trigger case.
 */
void generic_lua_model::set_reg_mask_and_value_actions_from_cci(const std::string& cci_name, register_info_t& regs_info)
{
    auto children = gs::sc_cci_children(cci_name.c_str());
    if (children.empty()) return;
    for (std::string s : children) {
        if (std::count_if(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); })) {
            if (!gs::cci_get<uint32_t>(m_broker, cci_name + "." + s + ".mask", regs_info.mask)) {
                regs_info.mask = DEFAULT_REG_MASK;
            }
            if (gs::cci_get<uint32_t>(m_broker, cci_name + "." + s + ".value", regs_info.value)) {
                regs_info.value_set = true;
            }
            if (gs::cci_get<uint32_t>(m_broker, cci_name + "." + s + ".if_val", regs_info.if_val)) {
                regs_info.if_val_set = true;
            }
        }
    }
}

void generic_lua_model::register_input_signal(uint32_t sig_num, bool value, const data_t& data,
                                              const signals_t& signals)
{
    if (data.empty() && signals.empty()) {
        SCP_FATAL(()) << "dispatch: can't get data or output signals reactions to target signal: "
                      << target_signal_sockets[sig_num].basename();
    }
    if (m_signals.find(sig_num) != m_signals.end()) {
        m_signals.at(sig_num).insert(std::make_pair(value, std::make_pair(data, signals)));
    } else {
        auto r_pair = m_signals.insert(
            std::make_pair(sig_num, std::unordered_map<bool, reactions_t>{ { value, std::make_pair(data, signals) } }));
        if (!r_pair.second) {
            SCP_FATAL(()) << "dispatch: action for target signal " << target_signal_sockets[sig_num].basename()
                          << " can't be set!";
        }
    }
}

void generic_lua_model::reg_action_cb(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay,
                                      const register_info_t& regs_info)
{
    if (regs_info.if_val_set) {
        if (regs_info.action == actions::PRE_READ) {
            if (*(regs_info.reg_ptr) != regs_info.if_val) {
                return;
            }
        } else {
            if (!txn.get_data_ptr()) {
                SCP_FATAL(()) << "reg_action_cb: txn data is null for register: " << regs_info.reg_name << " !";
            }
            uint32_t txn_val;
            std::memcpy(&txn_val, txn.get_data_ptr(), sizeof(uint32_t));
            if (txn_val != regs_info.if_val) {
                return;
            }
        }
    }
    if (regs_info.value_set) {
        regs_info.reg_ptr->set_mask(DEFAULT_REG_MASK);
        *(regs_info.reg_ptr) = ((*(regs_info.reg_ptr) & ~regs_info.mask) | (regs_info.value & regs_info.mask));
        regs_info.reg_ptr->set_mask(regs_info.reg_mask);
    }
    write_data(regs_info.data, action_source::REGISTER_CB);
    trigger_signals(regs_info.signals, action_source::REGISTER_CB);
}

void generic_lua_model::exec_data_thread(action_source act_src)
{
    while (true) {
        wait(m_data_peq[act_src]->get_event());
        tlm::tlm_generic_payload* txn_ptr = nullptr;
        while ((txn_ptr = m_data_peq[act_src]->get_next_transaction()) != nullptr) {
            sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
            auto addr = txn_ptr->get_address();
            uint32_t val;
            std::memcpy(&val, txn_ptr->get_data_ptr(), sizeof(uint32_t));
            SCP_DEBUG(()) << "write to address: 0x" << std::hex << addr << " data: 0x" << std::hex << val;
            initiator_socket->b_transport(*txn_ptr, delay);
            auto is_resp_err = txn_ptr->is_response_error();
            m_data_pending_list[act_src].remove_if(
                [txn_ptr](const generic_lua_model_txn_payload& p_txn) { return p_txn.txn.get() == txn_ptr; });
            if (is_resp_err) {
                SCP_FATAL(()) << "Failed to send txn on address: 0x" << std::hex << addr;
            }
        }
    }
}
void generic_lua_model::exec_sig_thread(action_source act_src)
{
    while (true) {
        wait(m_signals_peq[act_src]->get_event());
        std::pair<uint32_t, bool>* sig_payload_ptr = nullptr;
        while ((sig_payload_ptr = m_signals_peq[act_src]->get_next_transaction()) != nullptr) {
            SCP_DEBUG(()) << "write to output signal: " << initiator_signal_sockets[sig_payload_ptr->first].basename()
                          << " value: " << std::boolalpha << sig_payload_ptr->second;
            initiator_signal_sockets[sig_payload_ptr->first]->write(sig_payload_ptr->second);
            m_signals_pending_list[act_src].remove_if([sig_payload_ptr](const generic_lua_model_signal_payload& p_sig) {
                return p_sig.sig_num_val_pair.get() == sig_payload_ptr;
            });
        }
    }
}

void generic_lua_model::reg_data_thread() { exec_data_thread(action_source::REGISTER_CB); }

void generic_lua_model::reg_signals_thread() { exec_sig_thread(action_source::REGISTER_CB); }

void generic_lua_model::sos_data_thread() { exec_data_thread(action_source::START_OF_SIM); }

void generic_lua_model::sos_signals_thread() { exec_sig_thread(action_source::START_OF_SIM); }

void generic_lua_model::target_sig_data_thread() { exec_data_thread(action_source::TARGET_SIGNAL); }

void generic_lua_model::target_sig_signals_thread() { exec_sig_thread(action_source::TARGET_SIGNAL); }

void generic_lua_model::trigger_signals(const signals_t& signals, action_source act_src)
{
    if (signals.empty()) return;
    for (const auto& sig : signals) {
        const auto& [t_ns, sig_num, val] = sig;
        SCP_DEBUG(()) << "will trigger signal: " << initiator_signal_sockets[sig_num].basename()
                      << " value: " << std::boolalpha << val << " after: " << t_ns.to_string();
        m_signals_pending_list[act_src].emplace_back(sig_num, val);
        auto& sig_payload = m_signals_pending_list[act_src].back();
        m_signals_peq[act_src]->notify(*sig_payload.sig_num_val_pair, t_ns);
    }
}

void generic_lua_model::write_data(const data_t& data, action_source act_src)
{
    if (data.empty()) return;
    for (const auto& entry : data) {
        const auto& [t_ns, addr, val] = entry;
        SCP_DEBUG(()) << "will write to address: 0x" << std::hex << addr << " value: 0x" << std::hex << val
                      << " after: " << t_ns.to_string();
        m_data_pending_list[act_src].emplace_back(addr, val);
        auto& pending_txn = m_data_pending_list[act_src].back();
        m_data_peq[act_src]->notify(*pending_txn.txn, t_ns);
    }
}

void generic_lua_model::before_end_of_elaboration()
{
    if (m_registers.empty()) return;
    for (auto& reg_info : m_registers) {
        auto cb = std::bind(&generic_lua_model::reg_action_cb, this, std::placeholders::_1, std::placeholders::_2,
                            reg_info);
        if (reg_info.action == actions::PRE_READ) {
            reg_info.reg_ptr->pre_read(cb);
        } else if (reg_info.action == actions::PRE_WRITE) {
            reg_info.reg_ptr->pre_write(cb);
        } else if (reg_info.action == actions::POST_READ) {
            reg_info.reg_ptr->post_read(cb);
        } else if (reg_info.action == actions::POST_WRITE) {
            reg_info.reg_ptr->post_write(cb);
        } else {
            SCP_FATAL(()) << "before_end_of_elaboration(): unknown action! possible actions are: [PRE_READ, "
                             "PRE_WRITE, POST_READ, POST_WRITE]!";
        }
    }
}
void generic_lua_model::start_of_simulation()
{
    if (m_registers.empty()) return;
    for (auto& reg_info : m_registers) {
        if (reg_info.is_default_value_set) {
            reg_info.reg_ptr->set_mask(DEFAULT_REG_MASK);
            *(reg_info.reg_ptr) = reg_info.default_value;
            reg_info.reg_ptr->set_mask(reg_info.reg_mask);
        }
    }
    if (m_start_of_sim_ts_ns.empty()) return;
    for (const auto& ts : m_start_of_sim_ts_ns) {
        write_data(ts.first, action_source::START_OF_SIM);
        trigger_signals(ts.second, action_source::START_OF_SIM);
    }
}

void generic_lua_model::target_signal_cb(int id, bool value)
{
    SCP_DEBUG(()) << "received write on target signal: " << target_signal_sockets[id].basename()
                  << " with value: " << std::boolalpha << value;
    auto sig = m_signals.at(id);
    auto sig_it = sig.find(value);
    if (sig_it != sig.end()) {
        auto data = sig_it->second.first;
        auto signals = sig_it->second.second;
        write_data(data, action_source::TARGET_SIGNAL);
        trigger_signals(signals, action_source::TARGET_SIGNAL);
    }
    /**
     * No need to use else clause here, as the target signal may be configured to trigger an action only for true
     * value but false was written, or vice versa.
     */
}
} // namespace gs
typedef gs::generic_lua_model generic_lua_model;
void module_register() { GSC_MODULE_REGISTER_C(generic_lua_model, sc_core::sc_object*, sc_core::sc_object*); }

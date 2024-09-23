/*
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GS_REGISTERS_H
#define GS_REGISTERS_H

#define SC_ALLOW_DEPRECATED_IEEE_API

#include <type_traits>

#include <systemc>
#include <cci_configuration>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/multi_passthrough_target_socket.h>
#include <scp/report.h>
#include <cciutils.h>
#include <tlm_sockets_buswidth.h>

namespace gs {

/**
 * @brief  Provide a class to provide b_transport callback
 *
 */
class tlm_fnct
{
public:
    typedef std::function<void(tlm::tlm_generic_payload&, sc_core::sc_time&)> TLMFUNC;

private:
    TLMFUNC m_fnct;

public:
    tlm_fnct(tlm_fnct&) = delete;
    tlm_fnct(TLMFUNC cb): m_fnct(cb) {}
    void operator()(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay) { m_fnct(txn, delay); }
    virtual void dummy() {} // force polymorphism
};

/**
 * @brief A class that encapsulates a simple target port and a set of b_transport lambda functions for pre/post
 * read/write. when b_transport is called port, the correct lambda's will be invoked
 *
 */
class port_fnct : public tlm_utils::simple_target_socket<port_fnct, DEFAULT_TLM_BUSWIDTH>
{
    SCP_LOGGER((), "register");
    std::vector<std::shared_ptr<tlm_fnct>> m_pre_read_fncts;
    std::vector<std::shared_ptr<tlm_fnct>> m_pre_write_fncts;
    std::vector<std::shared_ptr<tlm_fnct>> m_post_read_fncts;
    std::vector<std::shared_ptr<tlm_fnct>> m_post_write_fncts;
    cci::cci_param<bool> p_is_callback;
    bool m_in_callback = false;

    std::string my_name()
    {
        std::string n(name());
        n = n.substr(0, n.length() - strlen("_target_socket")); // get rid of last part of string
        n = n.substr(n.find_last_of(".") + 1);                  // take the last part.
        return n;
    }

    /* to be implemented !!!!*/
    unsigned int transport_dbg(tlm::tlm_generic_payload& txn) { return 1; }

    void b_transport(tlm::tlm_generic_payload& txn, sc_core::sc_time& delay)
    {
        if (m_pre_read_fncts.size() || m_pre_write_fncts.size() || m_post_read_fncts.size() ||
            m_post_write_fncts.size()) {
            if (!m_in_callback) {
                m_in_callback = true;
                switch (txn.get_response_status()) {
                case tlm::TLM_INCOMPLETE_RESPONSE:
                    switch (txn.get_command()) {
                    case tlm::TLM_READ_COMMAND:
                        if (m_pre_read_fncts.size()) {
                            SCP_INFO(())("Pre-Read callbacks: {}", my_name());
                            for (auto& cb : m_pre_read_fncts) (*cb)(txn, delay);
                        }
                        break;
                    case tlm::TLM_WRITE_COMMAND:
                        if (m_pre_write_fncts.size()) {
                            SCP_INFO(())("Pre-Write callbacks: {}", my_name());
                            for (auto& cb : m_pre_write_fncts) (*cb)(txn, delay);
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                case tlm::TLM_OK_RESPONSE:
                    switch (txn.get_command()) {
                    case tlm::TLM_READ_COMMAND:
                        if (m_post_read_fncts.size()) {
                            SCP_INFO(())("Post-Read callbacks: {}", my_name());
                            for (auto& cb : m_post_read_fncts) (*cb)(txn, delay);
                        }
                        break;
                    case tlm::TLM_WRITE_COMMAND:
                        if (m_post_write_fncts.size()) {
                            SCP_INFO(())("Post-Write callbacks: {}", my_name());
                            for (auto& cb : m_post_write_fncts) (*cb)(txn, delay);
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                default:
                    break;
                }
                m_in_callback = false;
            }
        }
        txn.set_dmi_allowed(false);
    }

    template <typename T>
    void is_a_do(std::shared_ptr<tlm_fnct> cb, std::function<void()> fn)
    {
        auto cbt = dynamic_cast<T*>(cb.get());
        if (cbt) fn();
    }

public:
    void pre_read(tlm_fnct::TLMFUNC cb) { m_pre_read_fncts.push_back(std::make_shared<tlm_fnct>(cb)); }
    void pre_write(tlm_fnct::TLMFUNC cb) { m_pre_write_fncts.push_back(std::make_shared<tlm_fnct>(cb)); }
    void post_read(tlm_fnct::TLMFUNC cb) { m_post_read_fncts.push_back(std::make_shared<tlm_fnct>(cb)); }
    void post_write(tlm_fnct::TLMFUNC cb) { m_post_write_fncts.push_back(std::make_shared<tlm_fnct>(cb)); }

    port_fnct() = delete;
    port_fnct(std::string name, std::string path_name)
        : simple_target_socket<port_fnct, DEFAULT_TLM_BUSWIDTH>((name + "_target_socket").c_str())
        , p_is_callback(path_name + ".target_socket.is_callback", true, "Is a callback (true)")
    {
        SCP_LOGGER_NAME().features[0] = parent(sc_core::sc_object::name()) + "." + name;
        SCP_TRACE(())("Constructor");
        p_is_callback = true;
        p_is_callback.lock();
        tlm_utils::simple_target_socket<port_fnct, DEFAULT_TLM_BUSWIDTH>::register_b_transport(this,
                                                                                               &port_fnct::b_transport);
        tlm_utils::simple_target_socket<port_fnct, DEFAULT_TLM_BUSWIDTH>::register_transport_dbg(
            this, &port_fnct::transport_dbg);
    }

protected:
    std::string parent(std::string name) { return name.substr(0, name.find_last_of('.')); }
};

/**
 * @brief  A proxy data class that stores it's value using a b_transport interface
 *  Data is passed by pointer, array indexing (operator[]) is supported.
 */
template <class TYPE>
class proxy_data_array
{
    scp::scp_logger_cache& SCP_LOGGER_NAME();
    TYPE* m_dmi = nullptr;

    void check_dmi()
    {
        tlm::tlm_generic_payload m_txn;
        tlm::tlm_dmi m_dmi_data;
        m_txn.set_byte_enable_length(0);
        m_txn.set_dmi_allowed(false);
        m_txn.set_command(tlm::TLM_IGNORE_COMMAND);
        m_txn.set_data_ptr(nullptr);
        m_txn.set_address(p_offset);
        m_txn.set_data_length(sizeof(TYPE) * p_number);
        m_txn.set_streaming_width(sizeof(TYPE) * p_number);
        m_txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        if (initiator_socket->get_direct_mem_ptr(m_txn, m_dmi_data)) {
            uint64_t start = m_dmi_data.get_start_address();
            unsigned char* ptr = m_dmi_data.get_dmi_ptr();
            ptr += (p_offset - start);
            sc_assert(m_dmi_data.get_end_address() >= start + p_offset + (p_number * sizeof(TYPE)));
            m_dmi = reinterpret_cast<TYPE*>(ptr);
        }
    }

public:
    std::string m_path_name;
    cci::cci_param<uint64_t> p_number;
    cci::cci_param<uint64_t> p_offset;
    cci::cci_param<uint64_t> p_size;
    cci::cci_param<bool> p_relative_addresses;

    tlm_utils::simple_initiator_socket<proxy_data_array, DEFAULT_TLM_BUSWIDTH> initiator_socket;

    void get(TYPE* dst, uint64_t idx = 0, uint64_t length = 1)
    {
        sc_assert(idx + length <= p_number);
        if (m_dmi) {
            memcpy(dst, &m_dmi[idx], sizeof(TYPE) * length);
            SCP_TRACE(())("Got value (DMI) : [{:#x}]", fmt::join(std::vector<TYPE>(&dst[0], &dst[length]), ","));
        } else {
            tlm::tlm_generic_payload m_txn;
            sc_core::sc_time dummy;
            m_txn.set_byte_enable_length(0);
            m_txn.set_dmi_allowed(false);
            m_txn.set_command(tlm::TLM_READ_COMMAND);
            m_txn.set_data_ptr(reinterpret_cast<unsigned char*>(dst));
            m_txn.set_address(p_offset + (sizeof(TYPE) * idx));
            m_txn.set_data_length(sizeof(TYPE) * length);
            m_txn.set_streaming_width(sizeof(TYPE) * length);
            m_txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
            initiator_socket->b_transport(m_txn, dummy);
            sc_assert(m_txn.get_response_status() == tlm::TLM_OK_RESPONSE);
            SCP_TRACE(())("Got value (transport) : [{:#x}]", fmt::join(std::vector<TYPE>(&dst[0], &dst[length]), ","));
            if (m_txn.is_dmi_allowed()) {
                check_dmi();
            }
        }
    }

    void set(TYPE* src, uint64_t idx = 0, uint64_t length = 1)
    {
        sc_assert(idx + length <= p_number);
        if (m_dmi) {
            SCP_TRACE(())("Set value (DMI) : [{:#x}]", fmt::join(std::vector<TYPE>(&src[0], &src[length]), ","));
            memcpy(&m_dmi[idx], src, sizeof(TYPE) * length);
        } else {
            tlm::tlm_generic_payload m_txn;
            sc_core::sc_time dummy;
            m_txn.set_byte_enable_length(0);
            m_txn.set_dmi_allowed(false);
            m_txn.set_command(tlm::TLM_WRITE_COMMAND);
            m_txn.set_data_ptr(reinterpret_cast<unsigned char*>(src));
            m_txn.set_address(p_offset + (sizeof(TYPE) * idx));
            m_txn.set_data_length(sizeof(TYPE) * length);
            m_txn.set_streaming_width(sizeof(TYPE) * length);
            m_txn.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
            SCP_TRACE(())("Set value (transport) : [{:#x}]", fmt::join(std::vector<TYPE>(&src[0], &src[length]), ","));
            initiator_socket->b_transport(m_txn, dummy);
            sc_assert(m_txn.get_response_status() == tlm::TLM_OK_RESPONSE);
            if (m_txn.is_dmi_allowed()) {
                check_dmi();
            }
        }
    }

    TYPE& operator[](int idx)
    {
        if (!m_dmi) {
            check_dmi();
            if (!m_dmi) {
                SCP_FATAL(())("Operator[] can only be used for DMI'able registers");
            }
        }
        SCP_TRACE(())("Access value (DMI) using operator [] at idx {}", idx);
        return m_dmi[idx];
    }
    void invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end) { m_dmi = nullptr; }

    proxy_data_array(scp::scp_logger_cache& logger, std::string name, std::string path_name, uint64_t _offset = 0,
                     uint64_t number = 1)
        : SCP_LOGGER_NAME()(logger)
        , m_path_name(path_name)
        , p_number(path_name + ".number", number, "number of elements in this register")
        , initiator_socket((name + "_initiator_socket").c_str())
        , p_offset(path_name + ".target_socket.address", _offset, "Offset of this register")
        , p_size(path_name + ".target_socket.size", sizeof(TYPE) * number, "size of this register")
        , p_relative_addresses(path_name + ".target_socket.relative_addresses", true,
                               "allow relative_addresses for this register")

    {
        initiator_socket.register_invalidate_direct_mem_ptr(this,
                                                            &gs::proxy_data_array<TYPE>::invalidate_direct_mem_ptr);
    }
};

/**
 * @brief  A proxy data class that stores it's value using a b_transport interface
 *  Data is passed by value.
 */
template <class TYPE = uint32_t>
class proxy_data : public proxy_data_array<TYPE>
{
    scp::scp_logger_cache& SCP_LOGGER_NAME();

public:
    TYPE get()
    {
        TYPE tmp;
        proxy_data_array<TYPE>::get(&tmp);
        return tmp;
    }
    operator TYPE() { return get(); }

    void set(TYPE value) { proxy_data_array<TYPE>::set(&value); }
    void operator=(TYPE value) { set(value); }

    proxy_data(scp::scp_logger_cache& logger, std::string name, std::string path_name, uint64_t offset = 0,
               uint64_t number = 1, uint64_t start = 0, uint64_t length = sizeof(TYPE) * 8)
        : proxy_data_array<TYPE>(logger, name, path_name, offset, number), SCP_LOGGER_NAME()(logger)
    {
        static_assert(std::is_unsigned<TYPE>::value, "Register types must be unsigned");
    }
};

/* forward declaration */
template <class TYPE = uint32_t>
class gs_bitfield;
template <class TYPE = uint32_t>
class gs_field;
/**
 * @brief Class that encapsulates a 'register' that proxies it's data via a tlm interface,
 * and uses callbacks (on a tlm interface)
 *
 */
template <class TYPE = uint32_t>
class gs_register : public port_fnct, public proxy_data<TYPE>
{
    SCP_LOGGER((), "register");

private:
    std::string m_regname;
    std::string m_path;
    uint64_t m_offset;
    uint64_t m_size;

public:
    gs_register() = delete;
    gs_register(std::string _name, std::string path = "", uint64_t offset = 0, uint64_t number = 1)
        : m_regname(_name)
        , m_path(path)
        , m_offset(offset)
        , m_size(sizeof(TYPE) * number)
        , port_fnct(_name, path)
        , proxy_data<TYPE>(SCP_LOGGER_NAME(), _name, path, offset, number)
    {
        std::string n(name());
        SCP_LOGGER_NAME().features[0] = parent(n) + "." + _name;
        n = n.substr(0, n.length() - strlen("_target_socket")); // get rid of last part of string
        SCP_TRACE((), n)("constructor : {} attching in {}", _name, path);
    }
    void operator=(TYPE value) { proxy_data<TYPE>::set(value); }
    operator TYPE() { return proxy_data<TYPE>::get(); }

    void operator=(gs_register<TYPE>& other) { proxy_data<TYPE>::set(proxy_data<TYPE>::get()); }
    void operator+=(TYPE other) { proxy_data<TYPE>::set(proxy_data<TYPE>::get() + other); }
    void operator-=(TYPE other) { proxy_data<TYPE>::set(proxy_data<TYPE>::get() - other); }
    void operator&=(TYPE other) { proxy_data<TYPE>::set(proxy_data<TYPE>::get() & other); }
    void operator|=(TYPE other) { proxy_data<TYPE>::set(proxy_data<TYPE>::get() | other); }
    void operator<<=(TYPE other) { proxy_data<TYPE>::set(proxy_data<TYPE>::get() << other); }
    void operator>>=(TYPE other) { proxy_data<TYPE>::set(proxy_data<TYPE>::get() >> other); }

    TYPE& operator[](int idx) { return proxy_data_array<TYPE>::operator[](idx); }

    gs_bitfield<TYPE> operator[](gs_field<TYPE>& f) { return gs_bitfield<TYPE>(*this, f); }
    std::string get_regname() const { return m_regname; }
    std::string get_path() const { return m_path; }
    uint64_t get_offset() const { return m_offset; }
    uint64_t get_size() const { return m_size; }
};

/**
 * @brief Proxy for bitfield access to a register.
 *
 */
template <class TYPE>
class gs_bitfield
{
    uint32_t start, length;
    gs_register<TYPE>& m_reg;

public:
    gs_bitfield(gs_register<TYPE>& r, uint32_t s, uint32_t l): m_reg(r), start(s), length(l) {}

    gs_bitfield(gs_register<TYPE>& r, gs_bitfield& f): m_reg(r), start(f.start), length(f.length) {}

    void operator=(TYPE value)
    {
        sc_assert(value < (1ull << length));
        m_reg &= ~(((1ull << length) - 1) << start);
        m_reg |= value << start;
    }
    operator TYPE() { return (m_reg >> start) & ((1ull << length) - 1); }
};

/**
 * @brief fields within registered encapsulated using a bitfield proxy.
 * This field is constructed from a specific bit field in a specific register. The bitfield itself may be re-used to use
 * the same field in another register.
 */
template <class TYPE>
class gs_field
{
    SCP_LOGGER();
    gs_register<TYPE>& m_reg;
    uint32_t m_bit_start;
    uint32_t m_bit_length;
    gs_bitfield<TYPE> m_bitfield;

public:
    gs_field(gs_register<TYPE>& reg, std::string name, uint32_t bit_start = 0, uint32_t bit_length = sizeof(TYPE) * 8)
        : m_reg(reg), m_bit_start(bit_start), m_bit_length(bit_length), m_bitfield(reg, m_bit_start, m_bit_length)
    {
        SCP_TRACE(())("gs_field constructor");
        if (m_bit_length == 0) SCP_FATAL(())("Can't find bit length for {}", name);
    }

    void operator=(TYPE value) { m_bitfield = value; }
    void operator=(gs_field<TYPE>& value) { m_bitfield = (TYPE)value; }

    operator TYPE() { return m_bitfield; }

    operator gs::gs_bitfield<TYPE>&() { return m_bitfield; }
};

} // namespace gs

#endif // GS_REGISTERS_H
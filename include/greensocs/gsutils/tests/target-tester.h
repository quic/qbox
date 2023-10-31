/*
 *  This file is part of libgsutils
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GREENSOCS_GSUTILS_TESTS_TARGET_TESTER_H
#define GREENSOCS_GSUTILS_TESTS_TARGET_TESTER_H

#include <cstring>
#include <functional>

#include <gtest/gtest.h>

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <greensocs/gsutils/tlm_sockets_buswidth.h>

/**
 * @class TargetTester
 *
 * @brief A TLM target to do testing on an initiator
 *
 * @details This class allows to test an initiator by providing helpers to
 * standard TLM operations. The class user can register various callbacks for
 * classical TLM forward path calls. The goal of those callback is to provide
 * an easy mean of accessing most often used data in the callback parameters
 * directly. The complete payload of the current transaction is still
 * accessible using the `get_cur_txn(_delay)` method.
 *
 * When not registering any callbacks, this class behaves as a dummy target,
 * responding correctly to transactions with the following behaviour:
 *   - Standard b_transport behaviour with out-of-bound check (based on
 *     mmio_size given at construct time). On a write commmand, the data buffer
 *     if filled with zeros
 *   - Standard transport_dbg behaviour with out-of-bound check, returning 0 on
 *     error, and the transaction data size on success. The transaction data
 *     buffer is filled with zeros on a write command.
 *   - DMI request default behaviour is to always return false;
 *
 * Regular TLM forward calls can be overriden when inheriting this class if one
 * need fine control over the transaction. However, Be aware that by doing so,
 * you'll loose the helpers functionnality of this class.
 */
class TargetTester : public sc_core::sc_module
{
public:
    using TlmGenericPayload = tlm::tlm_generic_payload;
    using TlmResponseStatus = tlm::tlm_response_status;
    using TlmDmi = tlm::tlm_dmi;

    using AccessCallbackFn = std::function<TlmResponseStatus(uint64_t addr, uint8_t* data, size_t len)>;
    using DebugAccessCallbackFn = std::function<int(uint64_t addr, uint8_t* data, size_t len)>;
    using GetDirectMemPtrCallbackFn = std::function<bool(uint64_t addr, TlmDmi&)>;

private:
    size_t m_mmio_size;

    /* Only valid within a transaction callback */
    TlmGenericPayload* m_cur_txn = nullptr;
    sc_core::sc_time* m_cur_txn_delay = nullptr;

    bool m_last_txn_valid = false;
    TlmGenericPayload m_last_txn;

    bool m_last_txn_delay_valid = false;
    sc_core::sc_time m_last_txn_delay;

    AccessCallbackFn m_read_cb;
    AccessCallbackFn m_write_cb;

    DebugAccessCallbackFn m_debug_read_cb;
    DebugAccessCallbackFn m_debug_write_cb;

    GetDirectMemPtrCallbackFn m_dmi_cb;

    /* Factorized version of b_transport and transport_dbg */
    template <class RET, RET DEFAULT_RET, RET ADDRESS_ERROR_RET, RET DEFAULT_CB_RET, class CB_FN>
    RET generic_access(TlmGenericPayload& txn, const CB_FN& read_cb, const CB_FN& write_cb)
    {
        using namespace tlm;

        uint64_t addr;
        uint8_t* ptr;
        size_t len;
        RET ret = DEFAULT_RET;

        m_cur_txn = &txn;

        if (txn.get_command() == TLM_IGNORE_COMMAND) {
            goto out;
        }

        if (txn.get_address() + txn.get_data_length() >= m_mmio_size) {
            ret = ADDRESS_ERROR_RET;
            goto out;
        }

        addr = txn.get_address();
        ptr = txn.get_data_ptr();
        len = txn.get_data_length();

        switch (txn.get_command()) {
        case TLM_READ_COMMAND:
            if (m_read_cb) {
                ret = read_cb(addr, ptr, len);
            } else {
                ret = DEFAULT_CB_RET;
            }
            break;

        case TLM_WRITE_COMMAND:
            if (m_write_cb) {
                ret = write_cb(addr, ptr, len);
            } else {
                std::memset(ptr, 0, len);
                ret = DEFAULT_CB_RET;
            }
            break;

        default:
            /* TLM_IGNORE_COMMAND already handled above */
            ADD_FAILURE();
        }

    out:
        m_cur_txn = nullptr;

        m_last_txn.deep_copy_from(txn);
        m_last_txn_valid = true;

        return ret;
    }

protected:
    virtual void b_transport(TlmGenericPayload& txn, sc_core::sc_time& delay)
    {
        using namespace tlm;

        TlmResponseStatus ret;

        m_cur_txn_delay = &delay;
        ret = generic_access<TlmResponseStatus, TLM_OK_RESPONSE, TLM_ADDRESS_ERROR_RESPONSE, TLM_OK_RESPONSE,
                             AccessCallbackFn>(txn, m_read_cb, m_write_cb);
        m_cur_txn_delay = nullptr;

        txn.set_response_status(ret);

        m_last_txn_delay = delay;
        m_last_txn_delay_valid = true;
    }

    virtual unsigned int transport_dbg(TlmGenericPayload& txn)
    {
        int ret = 0;

        ret = generic_access<int, 0, 0, -1, DebugAccessCallbackFn>(txn, m_debug_read_cb, m_debug_write_cb);

        if (ret == -1) {
            ret = txn.get_data_length();
        }

        return ret;
    }

    virtual bool get_direct_mem_ptr(TlmGenericPayload& txn, TlmDmi& dmi_data)
    {
        uint64_t addr;

        m_last_txn.deep_copy_from(m_last_txn);

        addr = txn.get_address();

        if (m_dmi_cb) {
            return m_dmi_cb(addr, dmi_data);
        } else {
            return false;
        }
    }

public:
    tlm_utils::simple_target_socket<TargetTester, DEFAULT_TLM_BUSWIDTH> socket;

    /**
     * @brief Construct a TargetTester object with a name and an MMIO size
     *
     * @param[in] n The name of the SystemC module
     * @param[in] mmio_size The size of the memory mapped I/O region of this component
     */
    TargetTester(const sc_core::sc_module_name& n, size_t mmio_size): sc_core::sc_module(n), m_mmio_size(mmio_size)
    {
        socket.register_b_transport(this, &TargetTester::b_transport);
        socket.register_transport_dbg(this, &TargetTester::transport_dbg);
        socket.register_get_direct_mem_ptr(this, &TargetTester::get_direct_mem_ptr);
    }

    virtual ~TargetTester() {}

    /**
     * @brief Register callback called on b_tranport read transaction
     */
    void register_read_cb(AccessCallbackFn cb) { m_read_cb = cb; }

    /**
     * @brief Register callback called on b_tranport write transaction
     */
    void register_write_cb(AccessCallbackFn cb) { m_write_cb = cb; }

    /**
     * @brief Register callback called on transport_dbg read transaction
     */
    void register_debug_read_cb(DebugAccessCallbackFn cb) { m_debug_read_cb = cb; }

    /**
     * @brief Register callback called on transport_dbg write transaction
     */
    void register_debug_write_cb(DebugAccessCallbackFn cb) { m_debug_write_cb = cb; }

    /**
     * @brief Register a callback called on a get_direct_mem_ptr call
     */
    void register_get_direct_mem_ptr_cb(GetDirectMemPtrCallbackFn cb) { m_dmi_cb = cb; }

    /**
     * @brief Return true if the copy of the last transaction is valid
     *
     * @detail This method can be used to check whether this target effectively
     * received a transaction or not. It will return true if the internal copy
     * of the last transaction is valid. Note that this flag is reset when
     * calling the `get_last_txn` method.
     *
     * @return true if the copy of the last transaction is valid
     */
    bool last_txn_is_valid() const { return m_last_txn_valid; }

    /**
     * @brief Return a copy of the last transaction payload
     *
     * @detail This method returns an internal copy of the last transaction
     * payload. An internal flag checks whether the payload is valid or not.
     * Calling this method actually reset the flag so calling it two time in a
     * row will trigger a test failure. This ensures that you actually got the
     * transaction you expected to get.
     */
    const TlmGenericPayload& get_last_txn()
    {
        EXPECT_TRUE(m_last_txn_valid);
        m_last_txn_valid = false;

        return m_last_txn;
    }

    /**
     * @brief Return a copy of the last transaction delay value
     *
     * @detail This method returns an internal copy of the last transaction
     * delay value. An internal flag checks whether the delay value is valid or
     * not.  Calling this method actually reset the flag so calling it two time
     * in a row will trigger a test failure. This ensures that you actually got
     * the transaction you expected to get.
     */
    const sc_core::sc_time& get_last_txn_delay()
    {
        EXPECT_TRUE(m_last_txn_delay_valid);
        m_last_txn_delay_valid = false;

        return m_last_txn_delay;
    }

    /**
     * @brief Get the current transaction payload
     *
     * @details This method returns the payload of the transaction in progress.
     * It must be could from within a transaction callback only. The
     * transaction payload can be altered if needed.
     *
     * @return the current transaction payload
     */
    TlmGenericPayload& get_cur_txn()
    {
        EXPECT_TRUE(m_cur_txn != nullptr);
        return *m_cur_txn;
    }

    /**
     * @brief Get the current transaction delay
     *
     * @details This method returns the delay value of the transaction in
     * progress.  It must be could from within a transaction callback only. The
     * transaction delay can be altered if needed.
     *
     * @return the current transaction delay
     */
    sc_core::sc_time& get_cur_txn_delay()
    {
        EXPECT_TRUE(m_cur_txn_delay != nullptr);
        return *m_cur_txn_delay;
    }
};

#endif

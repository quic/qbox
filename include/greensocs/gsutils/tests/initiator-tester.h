/*
 *  This file is part of libgsutils
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

#ifndef _GREENSOCS_GSUTILS_TESTS_INITIATOR_TESTER_H
#define _GREENSOCS_GSUTILS_TESTS_INITIATOR_TESTER_H

#include <functional>

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>

/**
 * @class InitiatorTester
 *
 * @brief A TLM initiator to do testing on a target
 *
 * @details This class allows to test a target by providing helpers to standard
 * TLM operations. Those helpers rangs from the most generic to the most
 * simplified one. The idea is to provide simple helpers for the most common
 * cases, while still allowing full flexibility if needed.
 *
 * The `prepare_txn` method can be overriden if needed when inheriting this
 * class, to customize the way payloads are filled before a transaction. One
 * can also use the *_with_txn helpers and provide an already filled
 * payload with e.g. an extension. Please note however that `prepare_txn` is
 * still called on the payload to fill compulsory fields (namely the address,
 * data pointer, data length and TLM command).
 *
 * Read/write helpers return the `tlm::tlm_response_status` value of the
 * resulting transaction. The DMI hint value of the last transaction (the
 * `is_dmi_allowed()` flag) can be retrieved using the `get_last_dmi_hint`
 * method.
 *
 * When using standard read/write helpers, one can specify the value of the
 * b_transport `delay` parameter, using the `set_next_txn_delay` method. This
 * delay value can then be retrieved after the transaction using the
 * `get_last_txn_delay` method (to check the value written back by the target).
 *
 * Some helpers have a `debug` argument defaulting to `false`, when set to
 * true, transport_dbg is called instead of b_transport on the socket. The
 * transport_dbg return value is accessible through the
 * `get_last_transport_debug_ret` method.
 *
 * Regarding DMI requests, one can use the `do_dmi_request` helper to do a
 * simple get_direct_mem_ptr call with only an address. The resulting
 * tlm::tlm_dmi data can be retrieved using the `get_last_dmi_data` method.
 *
 * One can also register a callback to catch DMI invalidations on the backward
 * path of the socket, using the `register_invalidate_direct_mem_ptr` method.
 */
class InitiatorTester : public sc_core::sc_module
{
public:
    using TlmGenericPayload = tlm::tlm_generic_payload;
    using TlmResponseStatus = tlm::tlm_response_status;
    using TlmDmi = tlm::tlm_dmi;

    using InvalidateDirectMemPtrFn = std::function<void(uint64_t, uint64_t)>;

private:
    sc_core::sc_time m_last_txn_delay;
    unsigned int m_last_transport_debug_ret;
    bool m_last_dmi_hint = false;

    TlmDmi m_last_dmi_data;

    InvalidateDirectMemPtrFn m_dmi_inval_cb;

    void invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end) {
        if (m_dmi_inval_cb) {
            m_dmi_inval_cb(start, end);
        }
    }

protected:
    virtual void prepare_txn(TlmGenericPayload& txn, bool is_read, uint64_t addr, uint8_t* data,
                             size_t len) {
        using namespace tlm;

        tlm_command cmd = is_read ? TLM_READ_COMMAND : TLM_WRITE_COMMAND;

        txn.set_address(addr);
        txn.set_data_length(len);
        txn.set_data_ptr(data);
        txn.set_command(cmd);
        txn.set_streaming_width(len);
        txn.set_byte_enable_ptr(nullptr);
        txn.set_response_status(TLM_INCOMPLETE_RESPONSE);
    }

public:
    tlm_utils::simple_initiator_socket<InitiatorTester> socket;

    InitiatorTester(const sc_core::sc_module_name& n): sc_core::sc_module(n) {
        socket.register_invalidate_direct_mem_ptr(this,
                                                  &InitiatorTester::invalidate_direct_mem_ptr);
    }

    virtual ~InitiatorTester() {}

    /*
     * b_transport / transport_dbg helpers
     * -----------------------------------
     */

    /**
     * @brief Perform a b_transport TLM transaction using the `txn` TLM payload.
     *
     * @details This method performs a b_transport transaction using the `txn`
     * pre-filled payload. The transaction is not altered by this method so it
     * should be completely filled prior to calling this method.
     *
     * @param[inout] txn The payload to use for the transaction
     *
     * @return the tlm::tlm_response_status value of the transaction
     */
    TlmResponseStatus do_b_transport(TlmGenericPayload& txn) {
        socket->b_transport(txn, m_last_txn_delay);
        m_last_dmi_hint = txn.is_dmi_allowed();

        return txn.get_response_status();
    }

    /**
     * @brief Perform a transport_dbg TLM transaction using the `txn` TLM payload.
     *
     * @details This method performs a transport_dbg transaction using the `txn`
     * pre-filled payload. The transaction is not altered by this method so it
     * should be completely filled prior to calling this method.
     *
     * @param[inout] txn The payload to use for the transaction
     *
     * @return the tlm::tlm_response_status value of the transaction
     */
    TlmResponseStatus do_transport_dbg(TlmGenericPayload& txn) {
        m_last_transport_debug_ret = socket->transport_dbg(txn);

        return txn.get_response_status();
    }

    /**
     * @brief Perform a TLM transaction using the `txn` TLM payload.
     *
     * @details This method performs a transaction using the `txn` pre-filled
     * payload. The transaction is not altered by this method so it should be
     * completely filled prior to calling this method.
     *
     * @param[inout] txn The payload to use for the transaction
     * @param[in] debug Perform a transport_dbg instead of a b_transport
     *
     * @return the tlm::tlm_response_status value of the transaction
     */
    TlmResponseStatus do_transaction(TlmGenericPayload& txn, bool debug = false) {
        if (debug) {
            return do_transport_dbg(txn);
        } else {
            return do_b_transport(txn);
        }
    }

    /**
     * @brief Perform a simple read into the buffer pointed by `data` with a
     * pre-set payload
     *
     * @details This method performs a read into the buffer pointed by `data`.
     * It uses the `txn` payload for the transaction, by overwriting the
     * address, data pointer, data lenght and command fields of it. Other field
     * are left untouched by this initiator (they could be altered by the
     * target).
     *
     * @param[inout] txn The payload to use for the transaction
     * @param[in] addr Address of the read
     * @param[out] data Pointer to the buffer where to store the read data
     * @param[in] len Length of the read
     * @param[in] debug Perform a transport_dbg instead of a b_transport
     *
     * @return the tlm::tlm_response_status value of the transaction
     */
    TlmResponseStatus do_read_with_txn_and_ptr(TlmGenericPayload& txn, uint64_t addr, uint8_t* data,
                                               size_t len, bool debug = false) {
        prepare_txn(txn, true, addr, data, len);
        return do_transaction(txn, debug);
    }

    /**
     * @brief Perform a simple write with data pointed by `data` with a pre-set
     * payload
     *
     * @details This method performs a write from the buffer pointed by `data`.
     * It uses the `txn` payload for the transaction, by overwriting the
     * address, data pointer, data lenght and command fields of it. Other field
     * are left untouched by this initiator (they could be altered by the
     * target).
     *
     * @param[in] addr Address of the write
     * @param[in] data Pointer to the data to write (note that this method does
     *                 not guarantee `data` won't be modified by the target. It
     *                 does not perform a prior copy to enforce this)
     * @param[in] len Length of the write
     * @param[in] debug Perform a transport_dbg instead of a b_transport
     *
     * @return the tlm::tlm_response_status value of the transaction
     */
    TlmResponseStatus do_write_with_txn_and_ptr(TlmGenericPayload& txn, uint64_t addr,
                                                const uint8_t* data, size_t len,
                                                bool debug = false) {
        prepare_txn(txn, false, addr, const_cast<uint8_t*>(data), len);
        return do_transaction(txn, debug);
    }

    /**
     * @brief Perform a simple read into the buffer pointed by `data`
     *
     * @param[in] addr Address of the read
     * @param[out] data Pointer to the buffer where to store the read data
     * @param[in] len Length of the read
     * @param[in] debug Perform a transport_dbg instead of a b_transport
     *
     * @return the tlm::tlm_response_status value of the transaction
     */
    TlmResponseStatus do_read_with_ptr(uint64_t addr, uint8_t* data, size_t len,
                                       bool debug = false) {
        TlmGenericPayload txn;

        return do_read_with_txn_and_ptr(txn, addr, data, len, debug);
    }

    /**
     * @brief Perform a simple write with data pointed by `data`
     *
     * @param[in] addr Address of the write
     * @param[in] data Pointer to the data to write (note that this method does
     *                 not guarantee `data` won't be modified by the target. It
     *                 does not perform a prior copy to enforce this)
     * @param[in] len Length of the write
     * @param[in] debug Perform a transport_dbg instead of a b_transport
     *
     * @return the tlm::tlm_response_status value of the transaction
     */
    TlmResponseStatus do_write_with_ptr(uint64_t addr, const uint8_t* data, size_t len,
                                        bool debug = false) {
        TlmGenericPayload txn;

        return do_write_with_txn_and_ptr(txn, addr, data, len, debug);
    }

    /**
     * @brief Perform a simple read with a pre-set payload
     *
     * @details This method reads data into `data`. It uses the `txn` payload
     * for the transaction, by overwriting the address, data pointer, data
     * lenght and command fields of it. Other field are left untouched by this
     * initiator (they could be altered by the target).
     *
     * @param[inout] txn The payload to use for the transaction
     * @param[in] addr Address of the read
     * @param[out] data Where to retrieve the read value
     * @param[in] debug Perform a transport_dbg instead of a b_transport
     *
     * @return the tlm::tlm_response_status value of the transaction
     */
    template <class T>
    TlmResponseStatus do_read_with_txn(TlmGenericPayload& txn, uint64_t addr, T& data,
                                       bool debug = false) {
        uint8_t* ptr = reinterpret_cast<uint8_t*>(&data);

        return do_read_with_txn_and_ptr(txn, addr, ptr, sizeof(data), debug);
    }

    /**
     * @brief Perform a simple write with a pre-set payload
     *
     * @details This method performs a write from `data`. It uses the `txn`
     * payload for the transaction, by overwriting the address, data pointer,
     * data lenght and command fields of it. Other field are left untouched by
     * this initiator (they could be altered by the target).
     *
     * @param[inout] txn The payload to use for the transaction
     * @param[in] addr Address of the write
     * @param[in] data The value to write (note that this method does not
     *                 guarantee `data` won't be modified by the target. It
     *                 does not perform a prior copy to enforce this)
     * @param[in] debug Perform a transport_dbg instead of a b_transport
     *
     * @return the tlm::tlm_response_status value of the transaction
     */
    template <class T>
    TlmResponseStatus do_write_with_txn(TlmGenericPayload& txn, uint64_t addr, const T& data,
                                        bool debug = false) {
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&data);

        return do_write_with_txn_and_ptr(txn, addr, ptr, sizeof(data), debug);
    }

    /**
     * @brief Perform a simple read into `data`
     *
     * @param[in] addr Address of the read
     * @param[out] data Where to retrieve the read value
     * @param[in] debug Perform a transport_dbg instead of a b_transport
     *
     * @return the tlm::tlm_response_status value of the transaction
     */
    template <class T>
    TlmResponseStatus do_read(uint64_t addr, T& data, bool debug = false) {
        uint8_t* ptr = reinterpret_cast<uint8_t*>(&data);

        return do_read_with_ptr(addr, ptr, sizeof(data), debug);
    }

    /**
     * @brief Perform a simple write
     *
     * @param[in] addr Address of the write
     * @param[in] data Data to write (note that this method does not
     *                 guarantee `data` won't be modified by the target. It
     *                 does not perform a prior copy to enforce this)
     * @param[in] debug Perform a transport_dbg instead of a b_transport
     *
     * @return the tlm::tlm_response_status value of the transaction
     */
    template <class T>
    TlmResponseStatus do_write(uint64_t addr, const T& data, bool debug = false) {
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&data);

        return do_write_with_ptr(addr, ptr, sizeof(data), debug);
    }

    /**
     * @brief Set the delay value to use for the next b_transport call
     *
     * @param[in] delay The delay value to use for the next b_transport call
     */
    void set_next_txn_delay(const sc_core::sc_time& delay) { m_last_txn_delay = delay; }

    /**
     * @brief Get the delay value resulting of the last b_transport call
     *
     * @return the delay value resulting of the last b_transport call
     */
    const sc_core::sc_time& get_last_txn_delay() const { return m_last_txn_delay; }

    /**
     * @brief Get the return value of the last transport_dbg call
     *
     * @return the return value of the last transport_dbg call
     */
    unsigned int get_last_transport_debug_ret() const { return m_last_transport_debug_ret; }

    /**
     * @brief Get the DMI hint value of the last transaction (the
     * is_dmi_allowed() flag in the payload)
     *
     * @return the DMI hint value of the last transaction
     */
    bool get_last_dmi_hint() const { return m_last_dmi_hint; }

    /*
     * get_direct_mem_ptr helpers
     * --------------------------
     */

    /**
     * @brief Perform a get_direct_mem_ptr call by specifying an address
     *
     * @details Perform a DMI request by specifying an address for the request.
     * The DMI data can be retrieved using the `get_last_dmi_data` method.
     *
     * @return the value returned by the get_direct_mem_ptr call
     */
    bool do_dmi_request(uint64_t addr) {
        TlmGenericPayload txn;

        prepare_txn(txn, true, addr, nullptr, 0);
        return socket->get_direct_mem_ptr(txn, m_last_dmi_data);
    }

    /**
     * @brief Get the DMI data returned by the last get_direct_mem_ptr call
     *
     * @return the DMI data returned by the last get_direct_mem_ptr call
     */
    const TlmDmi& get_last_dmi_data() const { return m_last_dmi_data; }

    /*
     * Backward interface helpers
     * --------------------------
     */

    /**
     * @brief Register a callback on invalidate_direct_mem_ptr event
     */
    void register_invalidate_direct_mem_ptr(InvalidateDirectMemPtrFn cb) { m_dmi_inval_cb = cb; }
};

#endif

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

#ifndef _GREENSOCS_GSUTILS_TLM_EXTENSIONS_EXCLUSIVE_ACCESS_H
#define _GREENSOCS_GSUTILS_TLM_EXTENSIONS_EXCLUSIVE_ACCESS_H

#include <vector>

#include <tlm>

/**
 * @class ExclusiveAccessTlmExtension
 *
 * @brief Exclusive load/store TLM extension
 *
 * @details Exclusive load/store TLM extension. It embeds an initiator ID
 * (InitiatorId) and a store status (ExclusiveStoreStatus).
 *
 * The initiator ID is meant to be composed by all the routers on the path that
 * support this extension. Each router can call add_hop on the extension with
 * an unique ID correponding to the initiator the request is comming from
 * (typically the index of the initiator on the router). The first initiator is
 * not required call add_hop since an empty InitiatorId is a perfectly valid ID
 * (in the case the initiator would be directly connected to a target, without
 * routers in between). It can still do it if it needs to emit exclusive
 * transactions with different exclusive IDs.
 *
 * The store status is valid after a TLM_WRITE_COMMAND transaction and indicate
 * whether the exclusive store succeeded or not.
 */
class ExclusiveAccessTlmExtension : public tlm::tlm_extension<ExclusiveAccessTlmExtension>
{
public:
    enum ExclusiveStoreStatus {
        EXCLUSIVE_STORE_NA = 0,
        EXCLUSIVE_STORE_SUCCESS,
        EXCLUSIVE_STORE_FAILURE
    };

    class InitiatorId
    {
    private:
        std::vector<int> m_id;

    public:
        void add_hop(int id) { m_id.push_back(id); }

        bool operator<(const InitiatorId& o) const {
            if (m_id.size() != o.m_id.size()) {
                return m_id.size() < o.m_id.size();
            }

            auto it0 = m_id.begin();
            auto it1 = o.m_id.begin();
            for (; it0 != m_id.end(); it0++, it1++) {
                if (*it0 != *it1) {
                    return *it0 < *it1;
                }
            }

            return false;
        }

        bool operator==(const InitiatorId& o) const {
            if (m_id.size() != o.m_id.size()) {
                return false;
            }

            auto it0 = m_id.begin();
            auto it1 = o.m_id.begin();
            for (; it0 != m_id.end(); it0++, it1++) {
                if (*it0 != *it1) {
                    return false;
                }
            }

            return true;
        }

        bool operator!=(const InitiatorId& o) const { return !(*this == o); }
    };

private:
    InitiatorId m_id;
    ExclusiveStoreStatus m_store_sta = EXCLUSIVE_STORE_NA;

public:
    ExclusiveAccessTlmExtension() = default;
    ExclusiveAccessTlmExtension(const ExclusiveAccessTlmExtension&) = default;

    virtual tlm_extension_base* clone() const override {
        return new ExclusiveAccessTlmExtension(*this);
    }

    virtual void copy_from(const tlm_extension_base& ext) override {
        const ExclusiveAccessTlmExtension& other = static_cast<const ExclusiveAccessTlmExtension&>(
            ext);

        m_id = other.m_id;
        m_store_sta = other.m_store_sta;
    }

    void set_exclusive_store_success() { m_store_sta = EXCLUSIVE_STORE_SUCCESS; }

    void set_exclusive_store_failure() { m_store_sta = EXCLUSIVE_STORE_FAILURE; }

    ExclusiveStoreStatus get_exclusive_store_status() const { return m_store_sta; }

    void add_hop(int id) { m_id.add_hop(id); }

    const InitiatorId& get_initiator_id() const { return m_id; }
};

#endif

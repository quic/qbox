/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * SMMUv3 Memory Attributes TLM Extension
 */

#ifndef SMMUV3_MEMORY_ATTRS_H
#define SMMUV3_MEMORY_ATTRS_H

#include <tlm>
#include <sstream>
#include <cstdint>

namespace gs {

/**
 * TLM extension for ARM memory attributes extracted from page table descriptors.
 *
 * Consumers: memory controllers, caches, interconnect, performance models
 *
 * Usage (caller-owned, no heap allocation):
 *   smmuv3_memory_attrs_extension ext;
 *   ext.set_from_descriptor(desc, table_attrs);
 *   txn.set_extension(&ext);
 *   downstream->b_transport(txn, delay);
 *   txn.clear_extension<smmuv3_memory_attrs_extension>();
 *
 * The clone() override below uses `new` because the TLM extension API
 * mandates returning a raw tlm_extension_base*; the TLM core owns and
 * frees that pointer. Application code should never need to call new.
 */
class smmuv3_memory_attrs_extension : public tlm::tlm_extension<smmuv3_memory_attrs_extension>
{
public:
    // Memory type (decoded from AttrIndx + MAIR)
    enum MemoryType {
        DEVICE_nGnRnE = 0, // Device non-Gathering, non-Reordering, no Early Write Ack
        DEVICE_nGnRE = 1,  // Device non-Gathering, non-Reordering, Early Write Ack
        DEVICE_nGRE = 2,   // Device non-Gathering, Reordering, Early Write Ack
        DEVICE_GRE = 3,    // Device Gathering, Reordering, Early Write Ack
        NORMAL_NC = 4,     // Normal memory, Non-cacheable
        NORMAL_WT = 5,     // Normal memory, Write-Through cacheable
        NORMAL_WB = 6,     // Normal memory, Write-Back cacheable
    };

    // Shareability domain
    enum Shareability {
        NON_SHAREABLE = 0,
        RESERVED = 1,
        OUTER_SHAREABLE = 2,
        INNER_SHAREABLE = 3,
    };

    // Access permissions
    enum AccessPerm {
        PRIV_RW_USER_NONE = 0, // EL1+ read/write, EL0 no access
        PRIV_RW_USER_RW = 1,   // EL1+ read/write, EL0 read/write
        PRIV_RO_USER_NONE = 2, // EL1+ read-only, EL0 no access
        PRIV_RO_USER_RO = 3,   // EL1+ read-only, EL0 read-only
    };

    smmuv3_memory_attrs_extension()
        : m_memory_type(NORMAL_WB)
        , m_shareability(INNER_SHAREABLE)
        , m_access_perm(PRIV_RW_USER_RW)
        , m_attr_indx(0)
        , m_non_secure(true)
        , m_privileged_execute_never(false)
        , m_unprivileged_execute_never(false)
        , m_contiguous(false)
        , m_dirty_bit_modifier(false)
    {
    }

    // TLM extension interface
    virtual tlm::tlm_extension_base* clone() const override
    {
        auto* ext = new smmuv3_memory_attrs_extension();
        ext->m_memory_type = m_memory_type;
        ext->m_shareability = m_shareability;
        ext->m_access_perm = m_access_perm;
        ext->m_attr_indx = m_attr_indx;
        ext->m_non_secure = m_non_secure;
        ext->m_privileged_execute_never = m_privileged_execute_never;
        ext->m_unprivileged_execute_never = m_unprivileged_execute_never;
        ext->m_contiguous = m_contiguous;
        ext->m_dirty_bit_modifier = m_dirty_bit_modifier;
        return ext;
    }

    virtual void copy_from(const tlm::tlm_extension_base& ext) override
    {
        const auto& other = static_cast<const smmuv3_memory_attrs_extension&>(ext);
        m_memory_type = other.m_memory_type;
        m_shareability = other.m_shareability;
        m_access_perm = other.m_access_perm;
        m_attr_indx = other.m_attr_indx;
        m_non_secure = other.m_non_secure;
        m_privileged_execute_never = other.m_privileged_execute_never;
        m_unprivileged_execute_never = other.m_unprivileged_execute_never;
        m_contiguous = other.m_contiguous;
        m_dirty_bit_modifier = other.m_dirty_bit_modifier;
    }

    // Setters
    void set_memory_type(MemoryType type) { m_memory_type = type; }
    void set_shareability(Shareability sh) { m_shareability = sh; }
    void set_access_perm(AccessPerm ap) { m_access_perm = ap; }
    void set_attr_indx(uint8_t idx) { m_attr_indx = idx; }
    void set_non_secure(bool ns) { m_non_secure = ns; }
    void set_pxn(bool pxn) { m_privileged_execute_never = pxn; }
    void set_uxn(bool uxn) { m_unprivileged_execute_never = uxn; }
    void set_contiguous(bool c) { m_contiguous = c; }
    void set_dbm(bool dbm) { m_dirty_bit_modifier = dbm; }

    // Getters
    MemoryType get_memory_type() const { return m_memory_type; }
    Shareability get_shareability() const { return m_shareability; }
    AccessPerm get_access_perm() const { return m_access_perm; }
    uint8_t get_attr_indx() const { return m_attr_indx; }
    bool is_non_secure() const { return m_non_secure; }
    bool is_pxn() const { return m_privileged_execute_never; }
    bool is_uxn() const { return m_unprivileged_execute_never; }
    bool is_contiguous() const { return m_contiguous; }
    bool is_dbm() const { return m_dirty_bit_modifier; }

    // Convenience: extract from LPAE descriptor
    void set_from_descriptor(uint64_t desc, uint32_t table_attrs = 0)
    {
        // AttrIndx [5:2] - index into MAIR (would need MAIR to fully decode)
        m_attr_indx = (desc >> 2) & 0xF;

        // Shareability [9:8]
        m_shareability = static_cast<Shareability>((desc >> 8) & 0x3);

        // Access Permissions [7:6]
        m_access_perm = static_cast<AccessPerm>((desc >> 6) & 0x3);

        // NS bit [5]
        m_non_secure = (desc >> 5) & 0x1;

        // PXN/UXN from descriptor [54:53] and table attrs
        m_privileged_execute_never = ((desc >> 53) & 0x1) || ((table_attrs >> 3) & 0x1);
        m_unprivileged_execute_never = ((desc >> 54) & 0x1) || ((table_attrs >> 4) & 0x1);

        // Contiguous bit [52]
        m_contiguous = (desc >> 52) & 0x1;

        // DBM (Dirty Bit Modifier) [51]
        m_dirty_bit_modifier = (desc >> 51) & 0x1;

        // For memory type, we'd need to decode MAIR[AttrIndx]
        // For now, default to normal write-back
        m_memory_type = NORMAL_WB;
    }

    // For debugging
    std::string to_string() const
    {
        std::ostringstream oss;
        oss << "MemAttrs{type=";
        switch (m_memory_type) {
        case DEVICE_nGnRnE:
            oss << "Device-nGnRnE";
            break;
        case DEVICE_nGnRE:
            oss << "Device-nGnRE";
            break;
        case DEVICE_nGRE:
            oss << "Device-nGRE";
            break;
        case DEVICE_GRE:
            oss << "Device-GRE";
            break;
        case NORMAL_NC:
            oss << "Normal-NC";
            break;
        case NORMAL_WT:
            oss << "Normal-WT";
            break;
        case NORMAL_WB:
            oss << "Normal-WB";
            break;
        }
        oss << ", sh=";
        switch (m_shareability) {
        case NON_SHAREABLE:
            oss << "Non";
            break;
        case OUTER_SHAREABLE:
            oss << "Outer";
            break;
        case INNER_SHAREABLE:
            oss << "Inner";
            break;
        default:
            oss << "Reserved";
            break;
        }
        oss << ", ap=" << (int)m_access_perm << ", ns=" << m_non_secure << ", pxn=" << m_privileged_execute_never
            << ", uxn=" << m_unprivileged_execute_never << "}";
        return oss.str();
    }

private:
    MemoryType m_memory_type;
    Shareability m_shareability;
    AccessPerm m_access_perm;
    uint8_t m_attr_indx;
    bool m_non_secure;
    bool m_privileged_execute_never;
    bool m_unprivileged_execute_never;
    bool m_contiguous;
    bool m_dirty_bit_modifier;
};

} // namespace gs

#endif // SMMUV3_MEMORY_ATTRS_H

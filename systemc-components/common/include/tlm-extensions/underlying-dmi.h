/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBQBOX_TLM_EXTENSIONS_UNDERLYING_DMI_H
#define _LIBQBOX_TLM_EXTENSIONS_UNDERLYING_DMI_H

#include <tlm>

#include <libqemu-cxx/libqemu-cxx.h>
namespace gs {

/**
 * @class tlm_dmi_ex
 *
 * @details DMI extended with type of DMI and module calling information
 */
class tlm_dmi_ex : public tlm::tlm_dmi
{
public:
    enum dmi_type { dmi_mapped, dmi_nomap, dmi_valid, dmi_iommu } type;
    sc_core::sc_module* module;
    tlm_dmi_ex(sc_core::sc_module* m, tlm::tlm_dmi dmi, dmi_type t): module(m), tlm::tlm_dmi(dmi), type(t) {}
};

/**
 * @class UnderlyingDMITlmExtension
 *
 * @details Embeds a record of which target was accessed, in terms of a DMI record which shows it's start/end address in
 * the txn, which is populated as the network is traversed.
 */
class UnderlyingDMITlmExtension : public tlm::tlm_extension<UnderlyingDMITlmExtension>,
                                  public std::vector<gs::tlm_dmi_ex>
{
public:
    UnderlyingDMITlmExtension() = default;
    UnderlyingDMITlmExtension(const UnderlyingDMITlmExtension&) = default;

    virtual tlm_extension_base* clone() const override { return new UnderlyingDMITlmExtension(*this); }

    virtual void copy_from(tlm_extension_base const& ext) override
    {
        const UnderlyingDMITlmExtension& other = static_cast<const UnderlyingDMITlmExtension&>(ext);
        *this = other;
    }
    void add_dmi(sc_core::sc_module* m, tlm::tlm_dmi dmi, tlm_dmi_ex::dmi_type type)
    {
        tlm_dmi_ex d(m, dmi, type);
        push_back(d);
    }
    bool has_dmi(tlm_dmi_ex::dmi_type type)
    {
        for (auto d = begin(); d != end(); d++) {
            if (d->type == type) return true;
        }
        return false;
    }
    tlm::tlm_dmi& get_first(tlm_dmi_ex::dmi_type type)
    {
        for (auto d = begin(); d != end(); d++) {
            if (d->type == type) return *d;
        }
        SCP_FATAL("UnderlyingDMITlmExtension")("No iommu DMI data found\n");
    }
    tlm::tlm_dmi& get_last(tlm_dmi_ex::dmi_type type)
    {
        for (auto d = rbegin(); d != rend(); d++) {
            if (d->type == type) return *d;
        }
        SCP_FATAL("UnderlyingDMITlmExtension")("No iommu DMI data found\n");
    }
};

/*class UnderlyingDMITlmExtension : public tlm::tlm_extension<UnderlyingDMITlmExtension>
{

    tlm::tlm_dmi dmi_data;
    bool valid=false;
public:
    UnderlyingDMITlmExtension() = default;
    UnderlyingDMITlmExtension(const UnderlyingDMITlmExtension&) = default;

    virtual tlm_extension_base* clone() const override { return new UnderlyingDMITlmExtension(*this); }

    virtual void copy_from(tlm_extension_base const& ext) override
    {
        dmi_data = static_cast<const UnderlyingDMITlmExtension&>(ext).dmi_data;
        valid = static_cast<const UnderlyingDMITlmExtension&>(ext).valid;
    }

    void set_underlying_dmi(tlm::tlm_dmi& dmi)
    {
        valid = true;
        dmi_data = dmi;
    }
    tlm::tlm_dmi* get_underlying_dmi()
    {
        if (valid)
            return &dmi_data;
        else
            return nullptr;
    }
    bool is_valid() { return valid; }
};
*/

/**
 * @class target info recording TLM extension
 *
 * @details Embeds a record of which target was accessed, in terms of a DMI record which shows it's start/end address in
 * the txn, which is populated as the network is traversed.
 */
/*
class TargetInfoExtension : public tlm::tlm_extension<TargetInfoExtension>, public std::vector<tlm::tlm_dmi>
{
public:
    TargetInfoExtension() = default;
    TargetInfoExtension(const TargetInfoExtension&) = default;

public:
    virtual tlm_extension_base* clone() const override { return new TargetInfoExtension(*this); }

    virtual void copy_from(const tlm_extension_base& ext) override
    {
        const TargetInfoExtension& other = static_cast<const TargetInfoExtension&>(ext);
        *this = other;
    }
};
*/
} // namespace gs
#endif

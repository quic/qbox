//==============================================================================
//
// Copyright (c) 2023 Qualcomm Innovation Center, Inc.
//
// SPDX-License-Identifier: BSD-3-Clause
//
//==============================================================================

#include <dmi-manager.h>

std::ostream& operator<<(std::ostream& os, const QemuInstanceDmiManager::DmiRegion& region)
{
    os << "DmiRegion[0x" << std::hex << region.get_key() << "-0x" << region.get_end() << "]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const tlm::tlm_dmi& info)
{
    auto start = QemuInstanceDmiManager::DmiRegion::key_from_tlm_dmi(info);
    uint64_t size = (info.get_end_address() - info.get_start_address()) + 1;
    uint64_t end = start + size - 1;
    os << "TlmDmi[0x" << std::hex << start << "-0x" << end << "]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const QemuInstanceDmiManager::DmiRegionAlias& alias)
{
    os << "DmiRegionAlias[0x" << std::hex << alias.get_start() << "-0x" << alias.get_end() << "] at host offset 0x"
       << uintptr_t(alias.get_dmi_ptr());
    return os;
}
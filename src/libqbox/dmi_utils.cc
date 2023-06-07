//==============================================================================
//
// Copyright (c) 2023 Qualcomm Innovation Center, Inc.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted (subject to the limitations in the
// disclaimer below) provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above
//      copyright notice, this list of conditions and the following
//      disclaimer in the documentation and/or other materials provided
//      with the distribution.
//
//    * Neither the name Qualcomm Innovation Center nor the names of its
//      contributors may be used to endorse or promote products derived
//      from this software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
// GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
// HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//==============================================================================

#include "libqbox/dmi-manager.h"

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
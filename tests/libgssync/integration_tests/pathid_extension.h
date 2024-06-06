/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 * Author: GreenSocs 2022
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GREENSOCS_PATHID_EXTENSION_H
#define _GREENSOCS_PATHID_EXTENSION_H

#include <systemc>
#include <tlm>

namespace gs {

/**
 * @class Path recording TLM extension
 *
 * @brief Path recording TLM extension
 *
 * @details Embeds an  ID field in the txn, which is populated as the network
 * is traversed - see README.
 */

class PathIDExtension : public tlm::tlm_extension<PathIDExtension>, public std::vector<int>
{
public:
    PathIDExtension() = default;
    PathIDExtension(const PathIDExtension&) = default;

public:
    virtual tlm_extension_base* clone() const override { return new PathIDExtension(*this); }

    virtual void copy_from(const tlm_extension_base& ext) override
    {
        const PathIDExtension& other = static_cast<const PathIDExtension&>(ext);
        *this = other;
    }
};
} // namespace gs
#endif

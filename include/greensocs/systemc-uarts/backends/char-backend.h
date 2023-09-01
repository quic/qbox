/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <scp/report.h>

#pragma once

class CharBackend
{
protected:
    void* m_opaque;
    void (*m_receive)(void* opaque, const uint8_t* buf, int size);
    int (*m_can_receive)(void* opaque);

public:
    CharBackend()
    {
        SCP_DEBUG("CharBackend") << "Charbackend constructor";
        m_opaque = NULL;
        m_receive = NULL;
        m_can_receive = can_receive;
    }

    virtual void write(unsigned char c) = 0;

    static int can_receive(void* opaque) { return 0; }

    void register_receive(void* opaque, void (*receive)(void* opaque, const uint8_t* buf, int size),
                          int (*can_receive)(void* opaque))
    {
        m_opaque = opaque;
        m_receive = receive;
        m_can_receive = can_receive;
    }
};

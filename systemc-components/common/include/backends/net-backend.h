/*
 * Copyright (c) 2022 GreenSocs
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <macs/payload.h>

class NetworkBackend
{
protected:
    void* m_opaque;
    void (*m_receive)(void* opaque, Payload& frame);
    int (*m_can_receive)(void* opaque);

public:
    NetworkBackend()
    {
        m_opaque = NULL;
        m_receive = NULL;
        m_can_receive = NULL;
    }

    virtual void send(Payload& frame) = 0;

    void register_receive(void* opaque, void (*receive)(void* opaque, Payload& frame), int (*can_receive)(void* opaque))
    {
        m_opaque = opaque;
        m_receive = receive;
        m_can_receive = can_receive;
    }
};

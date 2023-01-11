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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA. You may obtain a copy of the Apache License at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#pragma once

#include "../components/payload.h"

class NetworkBackend {
protected:
  void *m_opaque;
  void (*m_receive)(void *opaque, Payload &frame);
  int (*m_can_receive)(void *opaque);

public:
  NetworkBackend() {
    m_opaque = NULL;
    m_receive = NULL;
    m_can_receive = NULL;
  }

  virtual void send(Payload &frame) = 0;

  void register_receive(void *opaque,
                        void (*receive)(void *opaque, Payload &frame),
                        int (*can_receive)(void *opaque)) {
    m_opaque = opaque;
    m_receive = receive;
    m_can_receive = can_receive;
  }
};

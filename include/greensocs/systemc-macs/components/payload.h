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

#include <inttypes.h>
#include <sys/types.h>
#include <systemc>

class Payload {
private:
  uint8_t *m_buffer;
  size_t m_size;
  size_t m_capacity;

public:
  Payload(size_t capacity) : m_size(0), m_capacity(capacity) {
    m_buffer = new uint8_t[capacity];
  }
  ~Payload() {
    if (m_buffer) {
      delete[] m_buffer;
    }
    m_buffer = nullptr;
  }

  size_t capacity() const { return m_capacity; }
  size_t size() const { return m_size; }
  uint8_t *data() const { return m_buffer; }

  void resize(size_t size) {
    assert(size <= m_capacity);
    m_size = size;
  }

private:
  Payload(const Payload &);
  Payload &operator=(const Payload &);
};

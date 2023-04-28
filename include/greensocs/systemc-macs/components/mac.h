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

#ifndef _MAC_H_
#define _MAC_H_

#include <cstring>
#include <inttypes.h>

#include <iostream>
#include <string>

class MACAddress {
private:
  uint8_t m_bytes[6];

public:
  MACAddress() { memset(m_bytes, 0, sizeof(m_bytes)); }

public:
  uint32_t lo() const;
  uint16_t hi() const;

  void set_lo(uint32_t value);
  void set_hi(uint16_t value);

public:
  void zero();

  inline void randomize() {
    for (int i = 0; i < 6; ++i) {
      m_bytes[i] = i;
    }
    m_bytes[0] &= ~((uint8_t)1); // clear multicast bit
  }

  bool set_from_str(const std::string &s);

public:
  bool operator==(const MACAddress &other) const {
    return memcmp(m_bytes, other.m_bytes, 6) == 0;
  }

  bool operator!=(const MACAddress &other) const {
    return !this->operator==(other);
  }

  bool operator<(const MACAddress &other) const {
    return memcmp(m_bytes, other.m_bytes, 6) < 0;
  }

  bool operator>(const MACAddress &other) const {
    return memcmp(m_bytes, other.m_bytes, 6) > 0;
  }

  bool operator<=(const MACAddress &other) const {
    return memcmp(m_bytes, other.m_bytes, 6) <= 0;
  }

  bool operator>=(const MACAddress &other) const {
    return memcmp(m_bytes, other.m_bytes, 6) >= 0;
  }

  uint8_t operator[](int i) const { return (i < 6) ? m_bytes[i] : 0; }
};

#endif

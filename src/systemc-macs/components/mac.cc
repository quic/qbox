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

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>

#include "greensocs/systemc-macs/components/mac.h"

uint32_t MACAddress::lo() const {
  return ((uint32_t)m_bytes[0] << 0) | ((uint32_t)m_bytes[1] << 8) |
         ((uint32_t)m_bytes[2] << 16) | ((uint32_t)m_bytes[3] << 24);
}

uint16_t MACAddress::hi() const {
  return ((uint16_t)m_bytes[4] << 0) | ((uint16_t)m_bytes[5] << 8);
}

void MACAddress::set_lo(uint32_t value) {
  m_bytes[0] = (value >> 0) & 0xff;
  m_bytes[1] = (value >> 8) & 0xff;
  m_bytes[2] = (value >> 16) & 0xff;
  m_bytes[3] = (value >> 24) & 0xff;
}

void MACAddress::set_hi(uint16_t value) {
  m_bytes[4] = (value >> 0) & 0xff;
  m_bytes[5] = (value >> 8) & 0xff;
}

void MACAddress::zero() { memset(m_bytes, 0, 6); }

bool MACAddress::set_from_str(const std::string &mac) {
  std::vector<std::string> strs;

  char *token;
  const char s[2] = ":";
  token = strtok((char *)mac.c_str(), s);
  while (token != NULL) {
    strs.push_back(token);
    token = strtok(NULL, s);
  }

  // boost::split(strs, mac, boost::is_any_of(":"));
  if (strs.size() != 6) {
    return false;
  }

  for (int i = 0; i < 6; i++) {
    std::string &s = strs[i];
    if (s.size() != 2 ||
        s.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) {
      return false;
    }
    m_bytes[i] = (uint8_t)std::strtoul(s.c_str(), NULL, 16);
  }

  return true;
}

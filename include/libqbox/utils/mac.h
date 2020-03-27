#ifndef _MAC_H_
#define _MAC_H_

#include <inttypes.h>
#include <cstring>

#include <iostream>
#include <string>

class MACAddress {
private:
    uint8_t m_bytes[6];

public:
    MACAddress() {
        memset(m_bytes, 0, sizeof(m_bytes));
    }

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
        m_bytes[0] &= ~((uint8_t) 1); // clear multicast bit
    }

    bool set_from_str(const std::string &s);

public:
    bool operator==(const MACAddress &other) const
    { return memcmp(m_bytes, other.m_bytes, 6) == 0; }

    bool operator!=(const MACAddress &other) const
    { return !this->operator==(other); }

    bool operator<(const MACAddress &other) const
    { return memcmp(m_bytes, other.m_bytes, 6) < 0; }

    bool operator>(const MACAddress &other) const
    { return memcmp(m_bytes, other.m_bytes, 6) > 0; }

    bool operator<=(const MACAddress &other) const
    { return memcmp(m_bytes, other.m_bytes, 6) <= 0; }

    bool operator>=(const MACAddress &other) const
    { return memcmp(m_bytes, other.m_bytes, 6) >= 0; }

    uint8_t operator[](int i) const { return (i < 6) ? m_bytes[i] : 0; }
};

#endif

#pragma once

#include <systemc>
#include <sys/types.h>
#include <inttypes.h>

class Payload {
private:
    uint8_t *m_buffer;
    size_t   m_size;
    size_t   m_capacity;

public:
    Payload(size_t capacity) : m_size(0), m_capacity(capacity) {
        m_buffer = new uint8_t[capacity];
    }
    ~Payload() {
        if (m_buffer) { delete[] m_buffer; }
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
    Payload(const Payload&);
    Payload& operator=(const Payload&);
};

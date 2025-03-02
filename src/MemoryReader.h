#ifndef QILBM_MEMORY_READER_H
#define QILBM_MEMORY_READER_H
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <cstring>

namespace qilbm {

class MemoryReader {
private:
    uint8_t *m_data;
    size_t m_size;
    size_t m_offset;

public:
    inline MemoryReader(uint8_t data[], size_t size) :
        m_data(data), m_size(size), m_offset(0) {}

    inline MemoryReader(const MemoryReader& other) :
        m_data(other.m_data), m_size(other.m_size), m_offset(other.m_offset) {}

    inline const uint8_t *data() const { return m_data; }
    inline size_t size() const { return m_size; }
    inline size_t offset() const { return m_offset; }
    inline bool is_empty() const { return m_offset >= m_size; }

    inline MemoryReader slice(size_t length) const {
        size_t end_offset = m_offset + length;
        if (end_offset > m_size) {
            end_offset = m_size;
        }

        return MemoryReader(m_data + m_offset, end_offset - m_offset);
    }

    inline bool read_i8(int8_t& value) {
        if (m_offset >= m_size) return false;
        value = (int8_t)m_data[m_offset];
        ++ m_offset;
        return true;
    }

    inline bool read_u8(int8_t& value) {
        if (m_offset >= m_size) return false;
        value = m_data[m_offset];
        ++ m_offset;
        return true;
    }

    inline bool read_i16be(int8_t& value) {
        if (m_offset + 2 > m_size) return false;
        value = (int16_t)(((uint16_t)m_data[m_offset] << 8) | (uint16_t)m_data[m_offset + 1]);
        m_offset += 2;
        return true;
    }

    inline bool read_u16be(int8_t& value) {
        if (m_offset + 2 > m_size) return false;
        value = ((uint16_t)m_data[m_offset] << 8) | (uint16_t)m_data[m_offset + 1];
        m_offset += 2;
        return true;
    }

    inline bool read_u32be(int8_t& value) {
        if (m_offset + 4 > m_size) return false;
        value = ((uint32_t)m_data[m_offset] << 24) | ((uint32_t)m_data[m_offset + 1] << 16) | ((uint32_t)m_data[m_offset + 2] << 8) | (uint32_t)m_data[m_offset + 3];
        m_offset += 4;
        return true;
    }

    inline bool read_fourcc(char fourcc[4]) {
        if (m_offset + 4 > m_size) return false;
        std::memcpy(fourcc, m_data + m_offset, 4);
        m_offset += 4;
        return true;
    }
};

}

#endif

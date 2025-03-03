#ifndef QILBM_COLOR_H
#define QILBM_COLOR_H
#pragma once

#include <stdint.h>
#include <cmath>

namespace qilbm {

#pragma pack(push)
class Color {
private:
    uint8_t m_r;
    uint8_t m_g;
    uint8_t m_b;

public:
    inline Color() : m_r(0), m_g(0), m_b(0) {}
    inline Color(uint8_t r, uint8_t g, uint8_t b) : m_r(r), m_g(g), m_b(b) {}

    inline uint8_t r() const { return m_r; };
    inline uint8_t g() const { return m_g; };
    inline uint8_t b() const { return m_b; };

    inline Color blend(const Color& other, double value) const {
        double inv = 1.0 - value;
        uint8_t r = (uint8_t)std::round((double)m_r * inv + (double)other.m_r * value);
        uint8_t g = (uint8_t)std::round((double)m_g * inv + (double)other.m_g * value);
        uint8_t b = (uint8_t)std::round((double)m_b * inv + (double)other.m_b * value);
        return Color(r, g, b);
    }
};
#pragma pack(pop)

}

#endif

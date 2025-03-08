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
    inline Color(const Color&) = default;
    inline Color(Color&&) = default;

    inline uint8_t r() const { return m_r; };
    inline uint8_t g() const { return m_g; };
    inline uint8_t b() const { return m_b; };

    inline void assign(uint8_t r, uint8_t g, uint8_t b) {
        m_r = r;
        m_g = g;
        m_b = b;
    }

    inline Color& operator=(const Color& other) {
        assign(other.r(), other.g(), other.b());
        return *this;
    }

    inline Color blend(const Color& other, double value) const {
        double inv = 1.0 - value;
        uint8_t r = (uint8_t)std::round((double)m_r * inv + (double)other.m_r * value);
        uint8_t g = (uint8_t)std::round((double)m_g * inv + (double)other.m_g * value);
        uint8_t b = (uint8_t)std::round((double)m_b * inv + (double)other.m_b * value);
        return Color(r, g, b);
    }
};
#pragma pack(pop)

#pragma pack(push)
class Color16 {
private:
    uint8_t m_0 : 4;
    uint8_t m_r : 4;
    uint8_t m_g : 4;
    uint8_t m_b : 4;

public:
    inline Color16() : m_0(0), m_r(0), m_g(0), m_b(0) {}
    inline Color16(uint16_t value) :
        m_0(0), m_r((value >> 8) & 0xF), m_g((value >> 4) & 0xF), m_b(value & 0xF) {}
    inline Color16(uint8_t r, uint8_t g, uint8_t b) :
        m_0(0), m_r(r & 0xF), m_g(g & 0xF), m_b(b & 0xF) {}
    inline Color16(const Color16&) = default;
    inline Color16(Color16&&) = default;

    inline uint8_t r() const { return m_r; };
    inline uint8_t g() const { return m_g; };
    inline uint8_t b() const { return m_b; };

    inline void assign(uint8_t r, uint8_t g, uint8_t b) {
        m_r = r;
        m_g = g;
        m_b = b;
    }

    inline void assign(uint16_t value) {
        m_r = (value >> 8) & 0xF;
        m_g = (value >> 4) & 0xF;
        m_b = value & 0xF;
    }

    inline Color16& operator=(const Color16& other) {
        assign(other.r(), other.g(), other.b());
        return *this;
    }

    inline Color16& operator=(uint16_t value) {
        assign(value);
        return *this;
    }

    inline Color16 blend(const Color16& other, double value) const {
        double inv = 1.0 - value;
        uint8_t r = (uint8_t)std::round((double)this->r() * inv + (double)other.r() * value);
        uint8_t g = (uint8_t)std::round((double)this->g() * inv + (double)other.g() * value);
        uint8_t b = (uint8_t)std::round((double)this->b() * inv + (double)other.b() * value);
        return Color16(r, g, b);
    }
};
#pragma pack(pop)

}

#endif

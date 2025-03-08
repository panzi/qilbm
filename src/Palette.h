#ifndef QILBM_PALETTE_H
#define QILBM_PALETTE_H
#pragma once

#include <array>
#include <vector>
#include <algorithm>

#include "Color.h"

namespace qilbm {

enum { LBM_CYCLE_RATE_DIVISOR = 280 };

class Cycle {
private:
    uint8_t m_low;
    uint8_t m_high;
    uint32_t m_rate;
    bool m_reverse;

public:
    Cycle(uint8_t low, uint8_t high, uint32_t rate, bool reverse) :
        m_low(low), m_high(high), m_rate(rate), m_reverse(reverse) {}

    inline uint8_t low() const { return m_low; }
    inline uint8_t high() const { return m_high; }
    inline uint32_t rate() const { return m_rate; }
    inline bool reverse() const { return m_reverse; }
};

class Palette {
private:
    std::array<Color, 256> m_data;

public:
    Palette() : m_data() {}

    inline const std::array<Color, 256>& data() const {
        return m_data;
    }

    inline size_t size() const {
        return m_data.size();
    }

    inline const Color& operator[](uint8_t index) const {
        return m_data[index];
    }

    inline Color& operator[](uint8_t index) {
        return m_data[index];
    }

    inline void rotate_left(uint8_t low, uint8_t high, uint32_t distance) {
        std::rotate(m_data.begin() + low, m_data.begin() + low + distance, m_data.begin() + high + 1);
    }

    inline void rotate_right(uint8_t low, uint8_t high, uint32_t distance) {
        std::rotate(m_data.begin() + low, m_data.begin() + high + 1 - distance, m_data.begin() + high + 1);
    }

    inline void clear() {
        m_data.fill(Color(0, 0, 0));
    }

    void apply_cycle(const Cycle& cycle, double now);
    void apply_cycle_blended(const Palette& palette, const Cycle& cycle, double now);
    void apply_cycles(const std::vector<Cycle>& cycles, double now);
    void apply_cycles_from(const Palette& palette, const std::vector<Cycle>& cycles, double now, bool blend);
};


class Palette16 {
private:
    std::array<Color16, 16> m_data;

public:
    Palette16() : m_data() {}

    inline const std::array<Color16, 16>& data() const {
        return m_data;
    }

    inline size_t size() const {
        return m_data.size();
    }

    inline const Color16& operator[](uint8_t index) const {
        return m_data[index];
    }

    inline Color16& operator[](uint8_t index) {
        return m_data[index];
    }

    inline void clear() {
        m_data.fill(Color16());
    }
};

}

#endif

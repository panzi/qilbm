#include "Palette.h"
#include <cmath>

using namespace qilbm;

void Palette::apply_cycle(const Cycle& cycle, double now) {
    uint8_t low = cycle.low();
    uint8_t high = cycle.high();
    uint32_t rate = cycle.rate();
    if (high > low && rate > 0) {
        double size = (double)(high - low + 1);
        double frate = (double)rate / (double)LBM_CYCLE_RATE_DIVISOR;
        uint32_t distance = (uint32_t)std::fmod(frate * now, size);
        if (cycle.reverse()) {
            rotate_left(low, high, distance);
        } else {
            rotate_right(low, high, distance);
        }
    }
}

void Palette::apply_cycle_blended(const Palette& palette, const Cycle& cycle, double now) {
    uint8_t low = cycle.low();
    uint8_t high = cycle.high();
    uint32_t rate = cycle.rate();
    if (high > low && rate > 0) {
        uint32_t size = (uint32_t)high - (uint32_t)low + 1;
        double fsize = (double)size;
        double frate = (double)rate / (double)LBM_CYCLE_RATE_DIVISOR;
        double fdistance = std::fmod(frate * now, fsize);
        uint32_t distance = (uint32_t)fdistance;
        double mid = fdistance - (double)distance;

        const Color* src = palette.m_data.data() + low;
        Color* dest = m_data.data() + low;

        if (cycle.reverse()) {
            for (uint32_t dest_index = 0; dest_index < size; ++ dest_index) {
                uint32_t src_index = dest_index + distance;
                uint32_t src_index1 = src_index % size;
                uint32_t src_index2 = (src_index + 1) % size;
                dest[dest_index] = src[src_index1].blend(src[src_index2], mid);
            }
        } else {
            double inv = 1.0 - mid;
            for (uint32_t src_index1 = 0; src_index1 < size; ++ src_index1) {
                uint32_t dest_index = (src_index1 + distance) % size;
                uint32_t src_index2 = (src_index1 + 1) % size;
                dest[dest_index] = src[src_index1].blend(src[src_index2], inv);
            }
        }
    }
}

void Palette::apply_cycles(const std::vector<Cycle>& cycles, double now) {
    for (auto& cycle : cycles) {
        apply_cycle(cycle, now);
    }
}

void Palette::apply_cycles_from(const Palette& palette, const std::vector<Cycle>& cycles, double now, bool blend) {
    m_data = palette.m_data;

    if (blend) {
        for (auto& cycle : cycles) {
            apply_cycle_blended(palette, cycle, now);
        }
    } else {
        apply_cycles(cycles, now);
    }
}

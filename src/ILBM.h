#ifndef QILBM_ILBM_H
#define QILBM_ILBM_H
#pragma once

#include <optional>
#include <vector>
#include <stdint.h>
#include <cmath>

#include "MemoryReader.h"

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

enum Result {
    Result_Ok = 0,
    Result_IOError = 1,
    Result_ParsingError = 2,
    Result_Unsupported = 3,
};

enum FileType {
    FileType_ILBM,
    FileType_PBM,
};

class BMHD {
private:
    uint16_t m_width;
    uint16_t m_height;
    int16_t m_x_origin;
    int16_t m_y_origin;
    uint8_t m_num_planes;
    uint8_t m_mask;
    uint8_t m_compression;
    uint8_t m_flags;
    uint16_t m_trans_color;
    uint8_t m_x_aspect;
    uint8_t m_y_aspect;
    int16_t m_page_width;
    int16_t m_page_height;

public:
    static const uint32_t SIZE = 20;

    BMHD() :
        m_width(0),
        m_height(0),
        m_x_origin(0),
        m_y_origin(0),
        m_num_planes(0),
        m_mask(0),
        m_compression(0),
        m_flags(0),
        m_trans_color(0),
        m_x_aspect(0),
        m_y_aspect(0),
        m_page_width(0),
        m_page_height(0)
    {}

    inline uint16_t width() const { return m_width; }
    inline uint16_t height() const { return m_height; }
    inline int16_t x_origin() const { return m_x_origin; }
    inline int16_t y_origin() const { return m_y_origin; }
    inline uint8_t num_planes() const { return m_num_planes; }
    inline uint8_t mask() const { return m_mask; }
    inline uint8_t compression() const { return m_compression; }
    inline uint8_t flags() const { return m_flags; }
    inline uint16_t trans_color() const { return m_trans_color; }
    inline uint8_t x_aspect() const { return m_x_aspect; }
    inline uint8_t y_aspect() const { return m_y_aspect; }
    inline int16_t page_width() const { return m_page_width; }
    inline int16_t page_height() const { return m_page_height; }

    Result read(MemoryReader& reader);
};

class BODY {
private:
    std::vector<uint8_t> m_data;
    std::vector<bool> m_mask;

public:
    BODY() : m_data{}, m_mask{} {}

    inline const std::vector<uint8_t>& data() const { return m_data; }
    inline const std::vector<bool>& mask() const { return m_mask; }

    Result read(MemoryReader& reader, FileType file_type, const BMHD& bmhd);

protected:
    void decode_line(const std::vector<uint8_t>& line, uint8_t mask, uint16_t width, size_t plane_len, size_t num_planes, FileType file_type);
};

class CMAP {
private:
    std::vector<Color> m_colors;

public:
    CMAP() : m_colors{} {}

    inline const std::vector<Color> colors() const { return m_colors; }

    Result read(MemoryReader& reader);
};

class CAMG {
private:
    uint32_t m_viewport_mode;

public:
    static const uint32_t SIZE = 4;

    CAMG() : m_viewport_mode(0) {}

    inline uint32_t viewport_mode() const { return m_viewport_mode; }

    Result read(MemoryReader& reader);
};

class CRNG {
private:
    uint16_t m_rate;
    uint16_t m_flags;
    uint8_t m_low;
    uint8_t m_high;

public:
    static const uint32_t SIZE = 8;

    CRNG() :
        m_rate(0),
        m_flags(0),
        m_low(0),
        m_high(0) {}

    inline uint16_t rate() const { return m_rate; }
    inline uint16_t flags() const { return m_flags; }
    inline uint8_t low() const { return m_low; }
    inline uint8_t high() const { return m_high; }

    Result read(MemoryReader& reader);
};

class CCRT {
private:
    int16_t m_direction;
    uint8_t m_low;
    uint8_t m_high;
    uint32_t m_delay_sec;
    uint32_t m_delay_usec;

public:
    static const uint32_t SIZE = 14;

    CCRT() :
        m_direction(0),
        m_low(0),
        m_high(0),
        m_delay_sec(0),
        m_delay_usec(0) {}

    inline int16_t direction() const { return m_direction; }
    inline uint8_t low() const { return m_low; }
    inline uint8_t high() const { return m_high; }
    inline uint32_t delay_sec() const { return m_delay_sec; }
    inline uint32_t delay_usec() const { return m_delay_usec; }

    Result read(MemoryReader& reader);
};

class ILBM {
private:
    FileType m_file_type;
    BMHD m_header;
    std::optional<CAMG> m_camg;
    std::optional<BODY> m_body;
    std::vector<CMAP> m_cmaps;
    std::vector<CRNG> m_crngs;
    std::vector<CCRT> m_ccrts;

public:
    static const uint32_t MIN_SIZE = BMHD::SIZE + 12;

    ILBM() :
        m_file_type(FileType_ILBM),
        m_header{},
        m_camg{},
        m_body{},
        m_cmaps{},
        m_crngs{},
        m_ccrts{} {}

    inline FileType file_type() const { return m_file_type; }
    inline const BMHD& header() const { return m_header; }
    inline const std::optional<CAMG>& camg() const { return m_camg; }
    inline const std::optional<BODY>& body() const { return m_body; }
    inline const std::vector<CMAP>& cmaps() const { return m_cmaps; }
    inline const std::vector<CRNG>& crngs() const { return m_crngs; }
    inline const std::vector<CCRT>& ccrts() const { return m_ccrts; }

    Result read(MemoryReader& reader);

    static bool can_read(MemoryReader& reader);
};

}

#endif

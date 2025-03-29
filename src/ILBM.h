#ifndef QILBM_ILBM_H
#define QILBM_ILBM_H
#pragma once

#include <optional>
#include <vector>
#include <memory>
#include <stdint.h>
#include <cmath>
#include <cstdio>
#include <string>

#include "MemoryReader.h"
#include "Color.h"
#include "Palette.h"

namespace qilbm {

enum Result {
    Result_Ok = 0,
    Result_IOError = 1,
    Result_ParsingError = 2,
    Result_Unsupported = 3,
};

const char *result_name(Result result);

enum FileType {
    FileType_ILBM,
    FileType_PBM,
};

const char *file_type_name(FileType file_type);

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

    inline void set_width(uint16_t width) { m_width = width; }
    inline void set_height(uint16_t height) { m_height = height; }
    inline void set_x_origin(int16_t x_origin) { m_x_origin = x_origin; }
    inline void set_y_origin(int16_t y_origin) { m_y_origin = y_origin; }
    inline void set_num_planes(uint8_t num_planes) { m_num_planes = num_planes; }
    inline void set_mask(uint8_t mask) { m_mask = mask; }
    inline void set_compression(uint8_t compression) { m_compression = compression; }
    inline void set_flags(uint8_t flags) { m_flags = flags; }
    inline void set_trans_color(uint16_t trans_color) { m_trans_color = trans_color; }
    inline void set_x_aspect(uint8_t x_aspect) { m_x_aspect = x_aspect; }
    inline void set_y_aspect(uint8_t y_aspect) { m_y_aspect = y_aspect; }
    inline void set_page_width(int16_t page_width) { m_page_width = page_width; }
    inline void set_page_height(int16_t page_height) { m_page_height = page_height; }

    Result read(MemoryReader& reader);

    void print(std::FILE* file) const;
};

class BODY {
private:
    std::vector<uint8_t> m_data;
    std::vector<bool> m_mask;

public:
    BODY() : m_data{}, m_mask{} {}

    inline const std::vector<uint8_t>& data() const { return m_data; }
    inline const std::vector<bool>& mask() const { return m_mask; }

    inline std::vector<uint8_t>& data() { return m_data; }
    inline std::vector<bool>& mask() { return m_mask; }

    Result read(MemoryReader& reader, FileType file_type, const BMHD& bmhd);

protected:
    void decode_line(const std::vector<uint8_t>& line, uint8_t mask, uint16_t width, size_t plane_len, size_t num_planes, FileType file_type);
};

class CMAP {
private:
    std::vector<Color> m_colors;

public:
    CMAP() : m_colors{} {}

    inline const std::vector<Color>& colors() const { return m_colors; }
    inline std::vector<Color>& colors() { return m_colors; }

    Result read(MemoryReader& reader);
};

class CAMG {
private:
    uint32_t m_viewport_mode;

public:
    enum {
        LACE  =    0x4,
        EHB   =   0x80, // extra half bright
        HAM   =  0x800, // hold and modify
        HIRES = 0x8000,
    };

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

    void print(std::FILE* file) const;
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

    void print(std::FILE* file) const;
};

class DYCP {
private:
    // TODO: find out what those mean
    // All references that I find say it should be "3 longwords",
    // but its 8 bytes in the files I can find and nowhere I can find
    // what the values mean.
    // See:
    // * https://wiki.amigaos.net/wiki/ILBM_IFF_Interleaved_Bitmap#ILBM.CTBL.DYCP
    // * http://amigadev.elowar.com/read/ADCD_2.1/Devices_Manual_guide/node027A.html
    uint32_t m_value1;
    uint32_t m_value2;

public:
    static const uint32_t SIZE = 8;

    DYCP() :
        m_value1(0), m_value2(0) {}

    inline uint32_t value1() const { return m_value1; }
    inline uint32_t value2() const { return m_value2; }

    Result read(MemoryReader& reader);
};

class CTBL {
private:
    std::vector<Palette> m_palettes;

public:
    CTBL() : m_palettes{} {}

    inline const std::vector<Palette>& palettes() const { return m_palettes; }
    inline std::vector<Palette>& palettes() { return m_palettes; }

    Result read(MemoryReader& reader);
};

class SHAM {
private:
    uint16_t m_version;
    std::vector<Palette> m_palettes;

public:
    SHAM() : m_version(0), m_palettes{} {}

    inline uint16_t version() const { return m_version; }

    inline const std::vector<Palette>& palettes() const { return m_palettes; }
    inline std::vector<Palette>& palettes() { return m_palettes; }

    Result read(MemoryReader& reader);
};

class TextChunk {
private:
    std::string m_content;

public:
    inline TextChunk() : m_content{} {};
    inline TextChunk(const std::string& content) : m_content(content) {}
    inline TextChunk(const char* content) : m_content(content) {}
    inline TextChunk(const char* content, size_t size) : m_content(content, size) {}
    inline TextChunk(const TextChunk&) = default;
    inline TextChunk(TextChunk&&) = default;

    inline const std::string& content() const { return m_content; }
    inline std::string& content() { return m_content; }

    inline void set_content(const std::string& content) {
        m_content = content;
    }

    inline void set_content(std::string&& content) {
        m_content = content;
    }

    inline void set_content(const char* content) {
        m_content = content;
    }

    inline void set_content(const char* content, size_t size) {
        m_content.assign(content, size);
    }

    Result read(MemoryReader& reader);
};

class ANNO : public TextChunk {};
class NAME : public TextChunk {};
class AUTH : public TextChunk {};
class Copy : public TextChunk {};

class ILBM {
private:
    FileType m_file_type;
    BMHD m_bmhd;
    std::optional<NAME> m_name;
    std::optional<AUTH> m_auth;
    std::optional<ANNO> m_anno;
    std::optional<Copy> m_copy;
    std::optional<CAMG> m_camg;
    std::optional<DYCP> m_dycp;
    std::unique_ptr<BODY> m_body;
    std::unique_ptr<CMAP> m_cmap;
    std::unique_ptr<CTBL> m_ctbl;
    std::unique_ptr<SHAM> m_sham;
    std::vector<CRNG> m_crngs;
    std::vector<CCRT> m_ccrts;

public:
    static const uint32_t MIN_SIZE = BMHD::SIZE + 12;

    ILBM() :
        m_file_type{FileType_ILBM},
        m_bmhd{},
        m_name{},
        m_auth{},
        m_anno{},
        m_copy{},
        m_camg{},
        m_dycp{},
        m_body{},
        m_cmap{},
        m_ctbl{},
        m_crngs{},
        m_ccrts{} {}

    inline FileType file_type() const { return m_file_type; }
    inline const BMHD& bmhd() const { return m_bmhd; }
    inline const NAME* name() const { return m_name ? &*m_name : nullptr; }
    inline const AUTH* auth() const { return m_auth ? &*m_auth : nullptr; }
    inline const ANNO* anno() const { return m_anno ? &*m_anno : nullptr; }
    inline const Copy* copy() const { return m_copy ? &*m_copy : nullptr; }
    inline const CAMG* camg() const { return m_camg ? &*m_camg : nullptr; }
    inline const DYCP* dycp() const { return m_dycp ? &*m_dycp : nullptr; }
    inline const BODY* body() const { return m_body.get(); }
    inline const CMAP* cmap() const { return m_cmap.get(); }
    inline const CTBL* ctbl() const { return m_ctbl.get(); }
    inline const std::vector<CRNG>& crngs() const { return m_crngs; }
    inline const std::vector<CCRT>& ccrts() const { return m_ccrts; }

    inline BMHD& bmhd() { return m_bmhd; }
    inline NAME* name() { return m_name ? &*m_name : nullptr; }
    inline AUTH* auth() { return m_auth ? &*m_auth : nullptr; }
    inline ANNO* anno() { return m_anno ? &*m_anno : nullptr; }
    inline Copy* copy() { return m_copy ? &*m_copy : nullptr; }
    inline CAMG* camg() { return m_camg ? &*m_camg : nullptr; }
    inline DYCP* dycp() { return m_dycp ? &*m_dycp : nullptr; }
    inline BODY* body() { return m_body.get(); }
    inline CMAP* cmap() { return m_cmap.get(); }
    inline CTBL* ctbl() { return m_ctbl.get(); }
    inline SHAM* sham() { return m_sham.get(); }
    inline std::vector<CRNG>& crngs() { return m_crngs; }
    inline std::vector<CCRT>& ccrts() { return m_ccrts; }

    inline void set_bmhd(BMHD bmhd) {
        m_bmhd = bmhd;
    }

    inline NAME& make_name() { return m_name.emplace(); }
    inline AUTH& make_auth() { return m_auth.emplace(); }
    inline ANNO& make_anno() { return m_anno.emplace(); }
    inline Copy& make_copy() { return m_copy.emplace(); }

    inline void clear_name() { m_name = std::nullopt; }
    inline void clear_auth() { m_auth = std::nullopt; }
    inline void clear_anno() { m_anno = std::nullopt; }
    inline void clear_copy() { m_copy = std::nullopt; }

    inline BODY& make_body() {
        m_body = std::make_unique<BODY>();
        return *m_body;
    }

    inline void clear_body() { m_body = nullptr; }

    Result read(MemoryReader& reader);

    static bool can_read(MemoryReader& reader);

    void get_cycles(std::vector<Cycle>& cycles) const;

    std::vector<Cycle> cycles() const {
        std::vector<Cycle> cycles;
        get_cycles(cycles);
        return cycles;
    }

    std::unique_ptr<Palette> palette() const;
};

class Renderer {
private:
    ILBM m_image;
    std::unique_ptr<Palette> m_palette;
    Palette m_cycled_palette;
    std::vector<Cycle> m_cycles;
    bool m_ham;

public:
    Renderer() :
        m_image(), m_palette(), m_cycled_palette(), m_cycles(), m_ham(false) {}

    inline const ILBM& image() const { return m_image; }
    inline const Palette* palette() const { return m_palette.get(); }
    inline const std::vector<Cycle>& cycles() const { return m_cycles; }
    inline bool is_ham() const { return m_ham; }
    inline bool is_animated() const { return m_palette && m_cycles.size() > 0; }

    Result read(MemoryReader& reader);
    void render(uint8_t* pixels, size_t pitch, double now, bool blend);
};

}

#endif

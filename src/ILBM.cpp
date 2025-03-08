#include "ILBM.h"
#include "Debug.h"
#include "Try.h"
#include <cstring>
#include <cassert>

#define GET_UINT16(BUF, INDEX) (((uint16_t)((BUF)[(INDEX)]) << 8) | (uint16_t)((BUF)[(INDEX) + 1]))

// generated with make_lookup_tables.py
static const uint8_t COLOR_LOOKUP_TABLE_1BIT[] = { 0, 255 };
static const uint8_t COLOR_LOOKUP_TABLE_2BITS[] = { 0, 85, 170, 255 };
static const uint8_t COLOR_LOOKUP_TABLE_3BITS[] = { 0, 36, 73, 109, 146, 182, 219, 255 };
static const uint8_t COLOR_LOOKUP_TABLE_4BITS[] = { 0, 17, 34, 51, 68, 85, 102, 119, 136, 153, 170, 187, 204, 221, 238, 255 };
static const uint8_t COLOR_LOOKUP_TABLE_5BITS[] = { 0, 8, 16, 25, 33, 41, 49, 58, 66, 74, 82, 90, 99, 107, 115, 123, 132, 140, 148, 156, 165, 173, 181, 189, 197, 206, 214, 222, 230, 239, 247, 255 };
static const uint8_t COLOR_LOOKUP_TABLE_6BITS[] = { 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 45, 49, 53, 57, 61, 65, 69, 73, 77, 81, 85, 89, 93, 97, 101, 105, 109, 113, 117, 121, 125, 130, 134, 138, 142, 146, 150, 154, 158, 162, 166, 170, 174, 178, 182, 186, 190, 194, 198, 202, 206, 210, 215, 219, 223, 227, 231, 235, 239, 243, 247, 251, 255 };
static const uint8_t COLOR_LOOKUP_TABLE_7BITS[] = { 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94, 96, 98, 100, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126, 129, 131, 133, 135, 137, 139, 141, 143, 145, 147, 149, 151, 153, 155, 157, 159, 161, 163, 165, 167, 169, 171, 173, 175, 177, 179, 181, 183, 185, 187, 189, 191, 193, 195, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221, 223, 225, 227, 229, 231, 233, 235, 237, 239, 241, 243, 245, 247, 249, 251, 253, 255 };

static const uint8_t *COLOR_LOOKUP_TABLES[8] = {
    NULL,
    COLOR_LOOKUP_TABLE_1BIT,
    COLOR_LOOKUP_TABLE_2BITS,
    COLOR_LOOKUP_TABLE_3BITS,
    COLOR_LOOKUP_TABLE_4BITS,
    COLOR_LOOKUP_TABLE_5BITS,
    COLOR_LOOKUP_TABLE_6BITS,
    COLOR_LOOKUP_TABLE_7BITS,
};

using namespace qilbm;

const char *qilbm::file_type_name(FileType file_type) {
    switch (file_type) {
        case FileType_ILBM: return "ILBM";
        case FileType_PBM:  return "PBM";
        default: return "Invalid file type";
    }
}

Result BMHD::read(MemoryReader& reader) {
    if (reader.remaining() < BMHD::SIZE) {
        LOG_DEBUG("truncated BMHD chunk: %zu < %u", reader.remaining(), BMHD::SIZE);
        return Result_ParsingError;
    }

    IO(reader.read_u16be(m_width));
    IO(reader.read_u16be(m_height));
    IO(reader.read_i16be(m_x_origin));
    IO(reader.read_i16be(m_y_origin));
    IO(reader.read_u8(m_num_planes));
    IO(reader.read_u8(m_mask));
    IO(reader.read_u8(m_compression));
    IO(reader.read_u8(m_flags));
    IO(reader.read_u16be(m_trans_color));
    IO(reader.read_u8(m_x_aspect));
    IO(reader.read_u8(m_y_aspect));
    IO(reader.read_i16be(m_page_width));
    IO(reader.read_i16be(m_page_height));

    reader.seek_relative(reader.remaining());

    return Result_Ok;
}

bool ILBM::can_read(MemoryReader& reader) {
    std::array<char, 4> fourcc;
    if (!reader.read_fourcc(fourcc)) {
        LOG_DEBUG("IO error reading FOURCC, remaining bytes: %zu", reader.remaining());
        return false;
    }

    if (std::memcmp(fourcc.data(), "FORM", 4) != 0) {
        LOG_DEBUG("illegal FOURCC: [0x%02x, 0x%02x, 0x%02x, 0x%02x] \"%c%c%c%c\"",
            (uint)fourcc[0], (uint)fourcc[1], (uint)fourcc[2], (uint)fourcc[3],
            fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
        return false;
    }

    uint32_t main_chunk_len = 0;
    if (!reader.read_u32be(main_chunk_len)) {
        LOG_DEBUG("IO error reading main_chunk_len, remaining bytes: %zu", reader.remaining());
        return false;
    }

    if (main_chunk_len < BMHD::SIZE + 4) {
        LOG_DEBUG("main chunk length too small: %u < %u", main_chunk_len, BMHD::SIZE + 4);
        return false;
    }

    if (!reader.read_fourcc(fourcc)) {
        LOG_DEBUG("IO error reading FOURCC, remaining bytes: %zu", reader.remaining());
        return false;
    }

    if (std::memcmp(fourcc.data(), "ILBM", 4) != 0 && std::memcmp(fourcc.data(), "PBM ", 4) != 0) {
        LOG_DEBUG("illegal FOURCC: [0x%02x, 0x%02x, 0x%02x, 0x%02x] \"%c%c%c%c\"",
            (uint)fourcc[0], (uint)fourcc[1], (uint)fourcc[2], (uint)fourcc[3],
            fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
        return false;
    }

    return true;
}

Result ILBM::read(MemoryReader& reader) {
    std::array<char, 4> fourcc;
    IO(reader.read_fourcc(fourcc));

    if (std::memcmp(fourcc.data(), "FORM", 4) != 0) {
        LOG_DEBUG("illegal FOURCC: [0x%02x, 0x%02x, 0x%02x, 0x%02x] \"%c%c%c%c\"",
            (uint)fourcc[0], (uint)fourcc[1], (uint)fourcc[2], (uint)fourcc[3],
            fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
        return Result_Unsupported;
    }

    uint32_t main_chunk_len = 0;
    IO(reader.read_u32be(main_chunk_len));

    if (main_chunk_len < BMHD::SIZE + 4) {
        LOG_DEBUG("main chunk length too small: %u < %u", main_chunk_len, BMHD::SIZE + 4);
        return Result_Unsupported;
    }

    IO(reader.read_fourcc(fourcc));

    if (std::memcmp(fourcc.data(), "ILBM", 4) == 0) {
        m_file_type = FileType_ILBM;
    } else if (std::memcmp(fourcc.data(), "PBM ", 4) == 0) {
        m_file_type = FileType_PBM;
    } else {
        LOG_DEBUG("unsupported file format: [0x%02x, 0x%02x, 0x%02x, 0x%02x] \"%c%c%c%c\"",
            (uint)fourcc[0], (uint)fourcc[1], (uint)fourcc[2], (uint)fourcc[3],
            fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
        return Result_Unsupported;
    }

    m_body = nullptr;
    m_cmap = nullptr;
    m_ctbl = nullptr;
    m_crngs.clear();
    m_ccrts.clear();
    m_camg = std::nullopt;
    m_dycp = std::nullopt;

    MemoryReader main_chunk_reader { reader, main_chunk_len - 4 };
    while (main_chunk_reader.remaining() > 0) {
        IO(main_chunk_reader.read_fourcc(fourcc));
        uint32_t chunk_len = 0;
        IO(main_chunk_reader.read_u32be(chunk_len));
        MemoryReader chunk_reader { main_chunk_reader, chunk_len };

        // std::printf("CHUNK: %c%c%c%c\n", fourcc[0], fourcc[1], fourcc[2], fourcc[3]);

        if (std::memcmp(fourcc.data(), "BMHD", 4) == 0) {
            TRY(m_header.read(chunk_reader));
        } else if (std::memcmp(fourcc.data(), "BODY", 4) == 0) {
            m_body = std::make_unique<BODY>();
            TRY(m_body->read(chunk_reader, m_file_type, m_header));
        } else if (std::memcmp(fourcc.data(), "CMAP", 4) == 0) {
            m_cmap = std::make_unique<CMAP>();
            TRY(m_cmap->read(chunk_reader));
        } else if (std::memcmp(fourcc.data(), "CRNG", 4) == 0) {
            CRNG& crng = m_crngs.emplace_back();
            TRY(crng.read(chunk_reader));
        } else if (std::memcmp(fourcc.data(), "CCRT", 4) == 0) {
            CCRT& ccrt = m_ccrts.emplace_back();
            TRY(ccrt.read(chunk_reader));
        } else if (std::memcmp(fourcc.data(), "CAMG", 4) == 0) {
            CAMG camg;
            TRY(camg.read(chunk_reader));
            m_camg = std::optional { std::move(camg) };
        } else if (std::memcmp(fourcc.data(), "DYCP", 4) == 0) {
            DYCP dycp;
            TRY(dycp.read(chunk_reader));
            m_dycp = std::optional { std::move(dycp) };
        } else if (std::memcmp(fourcc.data(), "CTBL", 4) == 0) {
            m_ctbl = std::make_unique<CTBL>();
            TRY(m_ctbl->read(chunk_reader));
        } else {
            // ignore unknown chunk
            // TODO: HAM, SHAM, ...
        }

        chunk_len += chunk_len & 1;
        main_chunk_reader.seek_relative(chunk_len);
    }

    if (m_camg) {
        auto viewport_mode = m_camg->viewport_mode();
        if (viewport_mode & CAMG::EHB) {
            if (!m_cmap) {
                m_cmap = std::make_unique<CMAP>();
            }

            auto& colors = m_cmap->colors();
            if (colors.size() < 64) {
                colors.resize(64, Color(0, 0, 0));
            }

            for (size_t index = 32; index < 64; ++ index) {
                auto color = colors[index - 32];
                colors[index] = Color(color.r() >> 1, color.g() >> 1, color.b() >> 1);
            }
        }

        if (viewport_mode & CAMG::HAM) {
            if (!m_cmap) {
                // HAM might access the palette, ensure it exists if HAM is true
                m_cmap = std::make_unique<CMAP>();
            }
        }
    }

    if (m_ctbl) {
        auto& palettes = m_ctbl->palettes();

        if (palettes.size() < (size_t)m_header.height()) {
            LOG_DEBUG(
                "fewer CTBL palettes than rows in image, extending with zeroed palettes: %zu < %u",
                palettes.size(), m_header.height());
            palettes.resize(m_header.height());
        }
    }

    return Result_Ok;
}

Result CMAP::read(MemoryReader& reader) {
    size_t num_colors = reader.remaining() / 3;

    m_colors.reserve(num_colors);
    for (size_t index = 0; index < num_colors; ++ index) {
        uint8_t rgb[3];
        IO(reader.read(rgb, sizeof(rgb)));
        m_colors.emplace_back(rgb[0], rgb[1], rgb[2]);
    }

    return Result_Ok;
}

Result CAMG::read(MemoryReader& reader) {
    if (reader.remaining() < CAMG::SIZE) {
        LOG_DEBUG("truncated CAMG chunk: %zu < %u", reader.remaining(), CAMG::SIZE);
        return Result_ParsingError;
    }

    IO(reader.read_u32be(m_viewport_mode));

    return Result_Ok;
}

Result CRNG::read(MemoryReader& reader) {
    if (reader.remaining()< CRNG::SIZE) {
        LOG_DEBUG("truncated CRNG chunk: %zu < %u", reader.remaining(), CRNG::SIZE);
        return Result_ParsingError;
    }

    uint16_t padding;
    IO(reader.read_u16be(padding));
    IO(reader.read_u16be(m_rate));
    IO(reader.read_u16be(m_flags));
    IO(reader.read_u8(m_low));
    IO(reader.read_u8(m_high));

    return Result_Ok;
}

void CRNG::print(std::FILE* file) const {
    std::fprintf(file,
        "CRNG { rate: %u, flags: 0x%x, low: %u, high: %u }",
        this->m_rate,
        this->m_flags,
        this->m_low,
        this->m_high
    );
}

Result CCRT::read(MemoryReader& reader) {
    if (reader.remaining()< CCRT::SIZE) {
        LOG_DEBUG("truncated CCRT chunk: %zu < %u", reader.remaining(), CCRT::SIZE);
        return Result_ParsingError;
    }

    IO(reader.read_i16be(m_direction));
    if (m_direction < -1 || m_direction > 1) {
        LOG_DEBUG("invalid CCRT direction: %d", m_direction);
        return Result_ParsingError;
    }

    IO(reader.read_u8(m_low));
    IO(reader.read_u8(m_high));
    IO(reader.read_u32be(m_delay_sec));
    IO(reader.read_u32be(m_delay_usec));

    uint16_t padding;
    IO(reader.read_u16be(padding));

    return Result_Ok;
}

void CCRT::print(std::FILE* file) const {
    std::fprintf(file,
        "CCRT { low: %u, high: %u, delay_sec: %u, delay_usec: %u }",
        this->m_low,
        this->m_high,
        this->m_delay_sec,
        this->m_delay_usec
    );
}

Result BODY::read(MemoryReader& reader, FileType file_type, const BMHD& header) {
    const size_t num_planes = header.num_planes();
    switch (num_planes) {
        case 1:
        case 4:
        case 8:
        case 24: // RGB888
        case 32: // RGBA8888
            break;

        default:
            if (file_type != FileType_ILBM || num_planes > 8) {
                LOG_DEBUG("unsupported number of bit planes: %zu", num_planes);
                return Result_Unsupported;
            }
    }
    const size_t width = header.width();
    const size_t height = header.height();
    const size_t pixel_count = width * height;

    const size_t plane_len = (width + 15) / 16 * 2;
    size_t line_len = num_planes * plane_len;
    if (header.mask() == 1) {
        line_len += plane_len;
    }
    std::vector<uint8_t> line;
    line.resize(line_len, 0);

    const size_t data_len = height * line_len;
    const size_t pixel_byte_len = pixel_count * ((num_planes + 7) / 8);

    m_data.reserve(pixel_byte_len);
    if (header.mask() == 1) {
        m_mask.reserve(pixel_count);
    }

    switch (header.compression()) {
        case 0:
            // uncompressed
            if (data_len > reader.remaining()) {
                LOG_DEBUG("truncated BODY chunk: %zu < %zu", reader.remaining(), data_len);
                return Result_ParsingError;
            }

            for (uint16_t y = 0; y < header.height(); ++ y) {
                IO(reader.read(line));
                decode_line(line, header.mask(), header.width(), plane_len, num_planes, file_type);
            }
            break;

        case 1:
            // compressed
            for (uint_fast16_t y = 0; y < header.height(); ++ y) {
                size_t pos = 0;
                uint8_t cmd = 0;
                while (pos < line_len) {
                    IO(reader.read_u8(cmd));
                    if (cmd < 128) {
                        size_t count = (size_t)cmd + 1;
                        size_t next_pos = pos + count;
                        if (next_pos > line_len) {
                            LOG_DEBUG("broken BODY compression, more data than fits into row: %zu > %zu", next_pos, line_len);
                            return Result_ParsingError;
                        }
                        IO(reader.read(line.data() + pos, count));
                        pos = next_pos;
                    } else if (cmd > 128) {
                        size_t count = 257 - (size_t)cmd;
                        uint8_t value;
                        IO(reader.read_u8(value));
                        size_t next_pos = pos + count;
                        if (next_pos > line_len) {
                            LOG_DEBUG("broken BODY compression, more data than fits into row: %zu > %zu", next_pos, line_len);
                            return Result_ParsingError;
                        }
                        std::fill(line.data() + pos, line.data() + next_pos, value);
                        pos = next_pos;
                    } else {
                        // some sources says 128 is EOF, other say its NOP
                    }
                    assert(pos <= line_len);
                    std::fill(line.data() + pos, line.data() + line_len, 0);
                }
                decode_line(line, header.mask(), header.width(), plane_len, num_planes, file_type);
            }
            break;

        case 2:
        {
            // VDAT compression
            m_data.resize(pixel_byte_len, 0);

            std::array<char, 4> fourcc;
            std::vector<uint8_t> buf;
            std::vector<uint8_t> decompr;
            decompr.reserve(pixel_byte_len);

            for (size_t plane_index = 0; plane_index < num_planes; ++ plane_index) {
                IO(reader.read_fourcc(fourcc));

                if (std::memcmp(fourcc.data(), "VDAT", 4) != 0) {
                    LOG_DEBUG("expected \"VDAT\" chunk but got: [0x%02x, 0x%02x, 0x%02x, 0x%02x] \"%c%c%c%c\"",
                        (uint)fourcc[0], (uint)fourcc[1], (uint)fourcc[2], (uint)fourcc[3],
                        fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
                    return Result_ParsingError;
                }

                uint32_t sub_chunk_len = 0;
                IO(reader.read_u32be(sub_chunk_len));
                if (sub_chunk_len > reader.remaining()) {
                    LOG_DEBUG("truncated compressed BODY chunk: %zu < %zu", reader.remaining(), (size_t)sub_chunk_len);
                    return Result_ParsingError;
                }

                MemoryReader sub_reader { reader, (size_t)sub_chunk_len };
                uint16_t cmd_cnt = 0;
                IO(sub_reader.read_u16be(cmd_cnt));

                if (cmd_cnt < 2) {
                    LOG_DEBUG("error in VDAT, cmd_cnt < 2: %u", cmd_cnt);
                    return Result_ParsingError;
                }
                cmd_cnt -= 2;
                size_t data_offset = cmd_cnt;
                const uint8_t *buf = sub_reader.data() + sub_reader.offset();
                const size_t buf_len = sub_reader.remaining();
                if (cmd_cnt > buf_len) {
                    LOG_DEBUG("truncated compressed BODY chunk: %u > %zu", cmd_cnt, buf_len);
                    return Result_ParsingError;
                }

                decompr.clear();
                for (size_t cmd_index = 0; cmd_index < cmd_cnt; ++ cmd_index) {
                    int8_t cmd = buf[cmd_index];

                    if (cmd == 0) { // load count from data, COPY
                        size_t next_offset = data_offset + 2;
                        if (next_offset < data_offset) {
                            LOG_DEBUG("overflow while decompressing BODY chunk: %zu < %zu", next_offset, data_offset);
                            return Result_ParsingError;
                        }
                        if (next_offset > buf_len) {
                            LOG_DEBUG("truncated compressed BODY chunk: %zu > %zu", next_offset, buf_len);
                            return Result_ParsingError;
                        }
                        size_t count = GET_UINT16(buf, data_offset);

                        data_offset = next_offset;
                        next_offset += count * 2;
                        if (next_offset < data_offset) {
                            LOG_DEBUG("overflow while decompressing BODY chunk: %zu < %zu", next_offset, data_offset);
                            return Result_ParsingError;
                        }
                        if (next_offset > buf_len) {
                            LOG_DEBUG("truncated compressed BODY chunk: %zu > %zu", next_offset, buf_len);
                            return Result_ParsingError;
                        }

                        std::copy(buf + data_offset, buf + next_offset, std::back_inserter(decompr));
                        data_offset = next_offset;
                    } else if (cmd == 1) { // load count from data, RLE
                        size_t next_offset = data_offset + 2;
                        if (next_offset < data_offset) {
                            LOG_DEBUG("overflow while decompressing BODY chunk: %zu < %zu", next_offset, data_offset);
                            return Result_ParsingError;
                        }
                        if (next_offset > buf_len) {
                            LOG_DEBUG("truncated compressed BODY chunk: %zu > %zu", next_offset, buf_len);
                            return Result_ParsingError;
                        }
                        uint_fast16_t count = GET_UINT16(buf, data_offset);

                        data_offset = next_offset;
                        next_offset += 2;
                        if (next_offset < data_offset) {
                            LOG_DEBUG("overflow while decompressing BODY chunk: %zu < %zu", next_offset, data_offset);
                            return Result_ParsingError;
                        }
                        if (next_offset > buf_len) {
                            LOG_DEBUG("truncated compressed BODY chunk: %zu > %zu", next_offset, buf_len);
                            return Result_ParsingError;
                        }

                        for (uint_fast16_t rle_index = 0; rle_index < count; ++ rle_index) {
                            decompr.emplace_back(buf[data_offset]);
                            decompr.emplace_back(buf[data_offset + 1]);
                        }
                        data_offset = next_offset;
                    } else if (cmd < 0) { // count = -cmd, COPY
                        size_t count = -(int_fast32_t)cmd;
                        size_t next_offset = data_offset + count * 2;
                        if (next_offset < data_offset) {
                            LOG_DEBUG("overflow while decompressing BODY chunk: %zu < %zu", next_offset, data_offset);
                            return Result_ParsingError;
                        }
                        if (next_offset > buf_len) {
                            LOG_DEBUG("truncated compressed BODY chunk: %zu > %zu", next_offset, buf_len);
                            return Result_ParsingError;
                        }

                        std::copy(buf + data_offset, buf + next_offset, std::back_inserter(decompr));
                        data_offset = next_offset;
                    } else { // cmd > 1: count = cmd, RLE
                        size_t count = cmd;
                        size_t next_offset = data_offset + 2;
                        if (next_offset < data_offset) {
                            LOG_DEBUG("overflow while decompressing BODY chunk: %zu < %zu", next_offset, data_offset);
                            return Result_ParsingError;
                        }
                        if (next_offset > buf_len) {
                            LOG_DEBUG("truncated compressed BODY chunk: %zu > %zu", next_offset, buf_len);
                            return Result_ParsingError;
                        }

                        for (uint_fast16_t rle_index = 0; rle_index < count; ++ rle_index) {
                            decompr.emplace_back(buf[data_offset]);
                            decompr.emplace_back(buf[data_offset + 1]);
                        }
                        data_offset = next_offset;
                    }
                    if (data_offset >= buf_len) {
                        break;
                    }
                }

                // TODO: fix 24 and 32 bit support
                for (size_t byte_index = 0; byte_index < decompr.size(); ++ byte_index) {
                    uint8_t value = decompr[byte_index];
                    size_t word_index = byte_index / 2;
                    size_t x = (word_index / height) * 16 + 8 * (byte_index & 1);
                    size_t y = word_index % height;

                    for (uint8_t bit = 0; bit < 8; ++ bit) {
                        size_t pixel_index = y * width + x + bit;
                        if (pixel_index >= m_data.size()) {
                            break;
                        }
                        m_data[pixel_index] |= ((value >> (7 - bit)) & 1) << plane_index;
                    }
                }

                reader.seek_relative(sub_chunk_len);
            }
            break;
        }
        default:
            LOG_DEBUG("unsupported compression flags: %u", header.compression());
            return Result_Unsupported;
    }

    if (header.mask() == 1 && m_mask.size() < pixel_count) {
        LOG_DEBUG("mask == 1, but didn't read enough mask bits: %zu < %zu", m_mask.size(), pixel_count);
        m_mask.resize(pixel_count, true);
    }

    return Result_Ok;
}

void BODY::decode_line(const std::vector<uint8_t>& line, uint8_t mask, uint16_t width, size_t plane_len, size_t num_planes, FileType file_type) {
    switch (file_type) {
        case FileType_ILBM:
            if (num_planes == 24 || num_planes == 32) {
                uint_fast16_t channels = num_planes / 8;
                size_t channel_len = (size_t)plane_len * 8;

                for (uint_fast16_t x = 0; x < width; ++ x) {
                    auto byte_offset = x / 8;
                    auto bit_offset = x % 8;
                    for (uint_fast16_t channel = 0; channel < channels; ++ channel) {
                        size_t offset = channel * channel_len + byte_offset;
                        uint8_t value = 0;
                        for (uint_fast16_t plane_index = 0; plane_index < 8; ++ plane_index) {
                            size_t byte_index = offset + plane_len * plane_index;
                            assert(byte_index < line.size());
                            uint8_t bit = (line[byte_index] >> (7 - bit_offset)) & 1;
                            value |= bit << plane_index;
                        }
                        m_data.emplace_back(value);
                    }
                }
            } else {
                for (uint_fast16_t x = 0; x < width; ++ x) {
                    size_t byte_offset = x / 8;
                    auto bit_offset = x % 8;
                    uint8_t value = 0;
                    for (size_t plane_index = 0; plane_index < num_planes; ++ plane_index) {
                        size_t byte_index = plane_len * plane_index + byte_offset;
                        uint8_t bit = (line[byte_index] >> (7 - bit_offset)) & 1;
                        value |= bit << plane_index;
                    }
                    m_data.emplace_back(value);
                }
            }
            break;

        case FileType_PBM:
            // TODO: test 1, 4, 24, and 32 bits
            switch (num_planes) {
                case 1:
                {
                    // XXX: don't know about the bit order!
                    for (uint_fast16_t index = 0; index < width / 8; ++ index) {
                        uint8_t byte = line[index];
                        m_data.emplace_back(byte & 1);
                        m_data.emplace_back((byte > 1) & 1);
                        m_data.emplace_back((byte > 2) & 1);
                        m_data.emplace_back((byte > 3) & 1);
                        m_data.emplace_back((byte > 4) & 1);
                        m_data.emplace_back((byte > 5) & 1);
                        m_data.emplace_back((byte > 6) & 1);
                        m_data.emplace_back((byte > 7) & 1);
                    }
                    uint_fast16_t rem = width % 8;
                    if (rem > 0) {
                        uint8_t byte = line[width / 8];
                        for (uint_fast16_t bit_index = 0; bit_index < rem; ++ bit_index) {
                            m_data.emplace_back((byte >> bit_index) & 1);
                        }
                    }
                    break;
                }
                case 4:
                    // XXX: don't know about the nibble order!
                    for (uint_fast16_t index = 0; index < width / 2; ++ index) {
                        uint8_t byte = line[index];
                        m_data.emplace_back(byte & 0xF);
                        m_data.emplace_back(byte >> 4);
                    }
                    break;

                case 8:
                    std::copy(line.data(), line.data() + width, std::back_inserter(m_data));
                    break;

                case 24:
                    std::copy(line.data(), line.data() + (size_t)width * 3, std::back_inserter(m_data));
                    break;

                case 32:
                    std::copy(line.data(), line.data() + (size_t)width * 4, std::back_inserter(m_data));
                    break;
            }
            break;

        default:
            LOG_DEBUG("illegal file type value: %d", file_type);
            assert(false);
            break;
    }

    if (mask == 1) {
        size_t offset = plane_len * num_planes;
        // inefficient, do 8 at a time except for last < 8
        for (uint16_t index = 0; index < width; ++ index) {
            uint8_t octet = line[offset + (size_t)index];
            uint16_t bit_offset = index % 8;
            // TODO: check bit order, might be different for PBM
            bool value = (octet >> (7 - bit_offset)) & 1;
            m_mask.emplace_back(value);
        }
    }
}

Result DYCP::read(MemoryReader& reader) {
    if (reader.remaining() < DYCP::SIZE) {
        LOG_DEBUG("truncated DYCP chunk: %zu < %u", reader.remaining(), DYCP::SIZE);
        return Result_ParsingError;
    }

    IO(reader.read_u32be(m_value1));
    IO(reader.read_u32be(m_value2));

    reader.seek_relative(reader.remaining());

    return Result_Ok;
}

Result CTBL::read(MemoryReader& reader) {
    size_t palette_count = reader.remaining() / 32;
    const uint8_t* data = reader.current();
    m_palettes.clear();

    size_t offset = 0;
    for (size_t palette_index = 0; palette_index < palette_count; ++ palette_index) {
        auto& palette = m_palettes.emplace_back();
        for (uint_fast8_t color_index = 0; color_index < 16; ++ color_index) {
            palette[color_index].assign(data[offset + 1], data[offset + 2], data[offset + 3]);
            offset += 4;
        }
    }

    return Result_Ok;
}

void ILBM::get_cycles(std::vector<Cycle>& cycles) const {
    for (auto& crng : this->crngs()) {
        auto low = crng.low();
        auto high = crng.high();
        auto rate = crng.rate();
        if (low < high && rate > 0) {
            auto flags = crng.flags();
            if ((flags & 1) != 0) {
                if (flags > 3) {
                    LOG_WARNING_OBJ("ILBMHandler::read(): Unsupported CRNG flags:", crng);
                }
                cycles.emplace_back(
                    low, high, rate, (flags & 2) != 0
                );
            } else if (flags != 0) {
                LOG_WARNING_OBJ("ILBMHandler::read(): Unsupported CRNG flags:", crng);
            }
        }
    }

    for (auto& ccrt : this->ccrts()) {
        auto low = ccrt.low();
        auto high = ccrt.high();
        auto direction = ccrt.direction();
        if (direction != 0 && low < high) {
            uint64_t usec = (uint64_t)ccrt.delay_sec() * 1000000 + (uint64_t)ccrt.delay_usec();
            uint64_t rate = usec * 8903 / 1000000;

            if (direction < -1 || direction > 1) {
                LOG_WARNING_OBJ("ILBMHandler::read(): Unsupported CCRT direction:", ccrt);
            }

            if (rate > 0) {
                cycles.emplace_back(
                    low, high, rate, direction > 0
                );
            }
        }
    }
}

std::unique_ptr<Palette> ILBM::palette() const {
    const auto* cmap = this->cmap();
    if (cmap == nullptr) {
        return nullptr;
    }

    auto palette = std::make_unique<Palette>();
    auto& pal = *palette;
    const auto& colors = cmap->colors();
    const size_t min_size = std::min(pal.size(), colors.size());
    for (size_t index = 0; index < min_size; ++ index) {
        pal[index] = colors[index];
    }

    return palette;
}

Result Renderer::read(MemoryReader& reader) {
    m_palette = nullptr;
    m_cycles.clear();
    m_cycled_palette.clear();

    Result result = m_image.read(reader);

    if (result != Result_Ok) {
        return result;
    }

    m_image.get_cycles(m_cycles);
    m_palette = m_image.palette();

    auto num_planes = m_image.header().num_planes();
    const auto* camg = m_image.camg();
    m_ham = camg && (camg->viewport_mode() & CAMG::HAM) && (num_planes >= 6 && num_planes <= 8);

    return result;
}

void Renderer::render(uint8_t* pixels, size_t pitch, double now, bool blend) {
    const auto& header = m_image.header();
    const auto width = header.width();
    const auto height = header.height();

    const auto num_planes = header.num_planes();

    const auto* body = m_image.body();
    const auto& data = body->data();
    const auto& mask = body->mask();
    const bool is_masked = header.mask() == 1;

    const uint8_t* ilbm_pixels = data.data();

    if (num_planes == 24) {
        if (is_masked) {
            size_t out_line_index = 0;
            size_t ilbm_index = 0;
            for (auto y = 0; y < height; ++ y) {
                size_t out_index = out_line_index;
                for (auto x = 0; x < width; ++ x) {
                    std::memcpy(pixels + out_index, ilbm_pixels + ilbm_index, 3);
                    ilbm_index += 3;
                    out_index += 4;
                }
                out_line_index += pitch;
            }
        } else {
            size_t ilbm_index = 0;
            size_t out_index = 0;
            size_t ilbm_line_len = (size_t)width * 3;
            for (auto y = 0; y < height; ++ y) {
                std::memcpy(pixels + out_index, ilbm_pixels + ilbm_index, ilbm_line_len);
                out_index += pitch;
                ilbm_index += ilbm_line_len;
            }
        }
    } else if (num_planes == 32) {
        size_t ilbm_index = 0;
        size_t out_index = 0;
        size_t ilbm_line_len = (size_t)width * 4;
        for (auto y = 0; y < height; ++ y) {
            std::memcpy(pixels + out_index, ilbm_pixels + ilbm_index, ilbm_line_len);
            out_index += pitch;
            ilbm_index += ilbm_line_len;
        }
    } else if (m_palette) {
        m_cycled_palette.apply_cycles_from(*m_palette, m_cycles, now, blend);

        size_t pixel_len = 3 + is_masked;

        // TODO: Does HAM without palettes exist? Is then the palette to be assumed all black?
        if (m_ham) {
            // HAM decoding http://www.etwright.org/lwsdk/docs/filefmts/ilbm.html
            const uint8_t payload_bits = num_planes - 2;
            const uint8_t ham_shift = 8 - payload_bits;
            const uint8_t ham_mask = (1 << ham_shift) - 1;
            const uint8_t payload_mask = 0xFF >> ham_shift;
            //const uint8_t *lookup_table = COLOR_LOOKUP_TABLES[payload_bits];

            size_t ilbm_index = 0;
            size_t out_line_index = 0;

            for (auto y = 0; y < height; ++ y) {
                size_t out_index = out_line_index;
                uint8_t r = 0;
                uint8_t g = 0;
                uint8_t b = 0;

                for (auto x = 0; x < width; ++ x) {
                    uint8_t code = data[ilbm_index];
                    uint8_t mode = code >> payload_bits;
                    uint8_t color_index = code & payload_mask;

                    switch (mode) {
                        case 0:
                        {
                            auto color = m_cycled_palette[color_index];
                            r = color.r();
                            g = color.g();
                            b = color.b();
                            break;
                        }
                        case 1:
                        {
                            // blue
                            b = (color_index << ham_shift) | (b & ham_mask);
                            break;
                        }
                        case 2:
                        {
                            // red
                            r = (color_index << ham_shift) | (r & ham_mask);
                            break;
                        }
                        case 3:
                        {
                            // green
                            g = (color_index << ham_shift) | (g & ham_mask);
                            break;
                        }
                        default:
                            // not possible
                            break;
                    }

                    pixels[out_index] = r;
                    pixels[out_index + 1] = g;
                    pixels[out_index + 2] = b;

                    out_index += pixel_len;
                    ++ ilbm_index;
                }
                out_line_index += pitch;
            }
        } else if (const auto* ctbl = m_image.ctbl()) {
            // XXX: does NOT work
            // TODO: check num_planes <= 4
            const auto& palettes = ctbl->palettes();
            size_t out_line_index = 0;
            size_t ilbm_index = 0;

            // TODO: if SHAM+LACED only change palette ever other line
            // const auto* camg = m_image.camg();
            // auto viewport_mode = camg ? camg->viewport_mode() : 0;
            // bool laced = viewport_mode & CAMB::LACED;

            for (auto y = 0; y < height; ++ y) {
                size_t out_index = out_line_index;
                const auto& palette = palettes[y];

                for (auto x = 0; x < width; ++ x) {
                    auto index = data[ilbm_index];
                    auto color = palette[index];
    
                    pixels[out_index]     = color.r() * 17;
                    pixels[out_index + 1] = color.g() * 17;
                    pixels[out_index + 2] = color.b() * 17;

                    ++ ilbm_index;
                    out_index += pixel_len;
                }
                out_line_index += pitch;
            }
#if 0
        } else if (const auto* ctbl = m_image.ctbl()) {
            // XXX: no idea how to process that
            // TODO: check num_planes == 4
            const auto& palettes = ctbl->palettes();

            const uint8_t payload_bits = num_planes - 2;
            const uint8_t ham_shift = 8 - payload_bits;
            const uint8_t ham_mask = (1 << ham_shift) - 1;
            const uint8_t payload_mask = 0xFF >> ham_shift;

            size_t ilbm_index = 0;
            size_t out_line_index = 0;

            for (auto y = 0; y < height; ++ y) {
                size_t out_index = out_line_index;
                const auto& palette = palettes[y];
                uint8_t r = 0;
                uint8_t g = 0;
                uint8_t b = 0;

                for (auto x = 0; x < width; ++ x) {
                    uint8_t code = data[ilbm_index];
                    uint8_t mode = code >> payload_bits;
                    uint8_t color_index = code & payload_mask;

                    switch (mode) {
                        case 0:
                        {
                            //auto color = m_cycled_palette[color_index];
                            auto color16 = palette[color_index];
                            r = color16.r() * 17;
                            g = color16.g() * 17;
                            b = color16.b() * 17;
                            break;
                        }
                        case 1:
                        {
                            // blue
                            b = (color_index << ham_shift) | (b & ham_mask);
                            break;
                        }
                        case 2:
                        {
                            // red
                            r = (color_index << ham_shift) | (r & ham_mask);
                            break;
                        }
                        case 3:
                        {
                            // green
                            g = (color_index << ham_shift) | (g & ham_mask);
                            break;
                        }
                        default:
                            // not possible
                            break;
                    }

                    pixels[out_index] = r;
                    pixels[out_index + 1] = g;
                    pixels[out_index + 2] = b;

                    out_index += pixel_len;
                    ++ ilbm_index;
                }
                out_line_index += pitch;
            }
#endif
        } else {
            size_t out_line_index = 0;
            size_t ilbm_index = 0;

            for (auto y = 0; y < height; ++ y) {
                size_t out_index = out_line_index;
                for (auto x = 0; x < width; ++ x) {
                    auto color = m_cycled_palette[data[ilbm_index]];
    
                    pixels[out_index] = color.r();
                    pixels[out_index + 1] = color.g();
                    pixels[out_index + 2] = color.b();

                    ++ ilbm_index;
                    out_index += pixel_len;
                }
                out_line_index += pitch;
            }
        }
    } else if (num_planes < 8) {
        // XXX: No idea if colors here should be done like in HAM? Need example files.
        // const uint8_t color_shift = 8 - num_planes;
        // const uint8_t color_mask = (1 << color_shift) - 1;

        const uint8_t *lookup_table = COLOR_LOOKUP_TABLES[num_planes];
        size_t pixel_len = 3 + is_masked;

        size_t out_line_index = 0;
        size_t ilbm_index = 0;

        for (auto y = 0; y < height; ++ y) {
            size_t out_index = out_line_index;
            for (auto x = 0; x < width; ++ x) {
                uint8_t value = lookup_table[data[ilbm_index]];
                std::memset(pixels + out_index, value, 3);

                out_index += pixel_len;
                ++ ilbm_index;
            }
            out_line_index += pitch;
        }
    } else {
        size_t ilbm_pixel_len = (num_planes + 7) / 8;
        size_t out_pixel_len = 3 + is_masked;

        size_t ilbm_index = 0;
        size_t out_line_index = 0;

        for (auto y = 0; y < height; ++ y) {
            size_t out_index = out_line_index;
            for (auto x = 0; x < width; ++ x) {
                uint8_t value = data[ilbm_index];
                std::memset(pixels + out_index, value, 3);

                out_index += out_pixel_len;
                ilbm_index += ilbm_pixel_len;
            }
            out_line_index += pitch;
        }
    }

    if (is_masked) {
        size_t out_line_index = 0;
        size_t mask_index = 0;

        for (auto y = 0; y < height; ++ y) {
            size_t out_index = out_line_index;
            for (auto x = 0; x < width; ++ x) {
                pixels[out_index + 3] = mask[mask_index] * 255;
                out_index += 4;
                ++ mask_index;
            }
            out_line_index += pitch;
        }
    }
}

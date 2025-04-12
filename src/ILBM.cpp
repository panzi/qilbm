#include "ILBM.h"
#include "Debug.h"
#include "Try.h"
#include <cstring>
#include <cassert>

#define GET_UINT16(BUF, INDEX) (((uint16_t)((BUF)[(INDEX)]) << 8) | (uint16_t)((BUF)[(INDEX) + 1]))
#define GET_UINT32(PTR) (((uint32_t)((PTR)[0]) << 24) | ((uint32_t)((PTR)[1]) << 16) | ((uint32_t)((PTR)[2]) << 8) | (uint32_t)((PTR)[3]))
#define EXTEND_4_TO_8_BIT(X) ((X) * 17)

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

const char *qilbm::result_name(Result result) {
    switch (result) {
        case Result_Ok:           return "Ok";
        case Result_IOError:      return "IO Error";
        case Result_ParsingError: return "Parsing Error";
        case Result_Unsupported:  return "Unsupported";
        default: return "Invalid result";
    }
}

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

void BMHD::print(std::FILE* file) const {
    std::fprintf(file,
        "BMHD {\n"
        "    width: %u,\n"
        "    height: %u,\n"
        "    x_origin: %d,\n"
        "    y_origin: %d,\n"
        "    num_planes: %u,\n"
        "    mask: %u,\n"
        "    compression: %u,\n"
        "    flags: %u,\n"
        "    trans_color: 0x%06x,\n"
        "    x_aspect: %u,\n"
        "    y_aspect: %u,\n"
        "    page_width: %d,\n"
        "    page_height: %d\n"
        "}",
        (unsigned int)m_width,
        (unsigned int)m_height,
        (int)m_x_origin,
        (int)m_y_origin,
        (unsigned int)m_num_planes,
        (unsigned int)m_mask,
        (unsigned int)m_compression,
        (unsigned int)m_flags,
        (unsigned int)m_trans_color,
        (unsigned int)m_x_aspect,
        (unsigned int)m_y_aspect,
        (int)m_page_width,
        (int)m_page_height
    );
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

Result ILBM::read(MemoryReader& reader, bool only_metadata) {
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
    m_sham = nullptr;
    m_pchg = nullptr;
    m_crngs.clear();
    m_ccrts.clear();
    m_camg = std::nullopt;
    m_dycp = std::nullopt;
    m_name = std::nullopt;
    m_auth = std::nullopt;
    m_anno = std::nullopt;
    m_copy = std::nullopt;

    MemoryReader main_chunk_reader { reader, main_chunk_len - 4 };
    while (main_chunk_reader.remaining() > 0) {
        IO(main_chunk_reader.read_fourcc(fourcc));
        uint32_t chunk_len = 0;
        IO(main_chunk_reader.read_u32be(chunk_len));
        MemoryReader chunk_reader { main_chunk_reader, chunk_len };

        // std::printf("CHUNK: %c%c%c%c\n", fourcc[0], fourcc[1], fourcc[2], fourcc[3]);

        if (std::memcmp(fourcc.data(), "BMHD", 4) == 0) {
            TRY(m_bmhd.read(chunk_reader));
        } else if (std::memcmp(fourcc.data(), "BODY", 4) == 0) {
            if (!only_metadata) {
                m_body = std::make_unique<BODY>();
                TRY(m_body->read(chunk_reader, m_file_type, m_bmhd));
            }
        } else if (std::memcmp(fourcc.data(), "CMAP", 4) == 0) {
            m_cmap = std::make_unique<CMAP>();
            PASS_IF(only_metadata, m_cmap->read(chunk_reader), { m_cmap = nullptr; });
        } else if (std::memcmp(fourcc.data(), "CRNG", 4) == 0) {
            CRNG& crng = m_crngs.emplace_back();
            PASS_IF(only_metadata, crng.read(chunk_reader), { m_crngs.pop_back(); });
        } else if (std::memcmp(fourcc.data(), "CCRT", 4) == 0) {
            CCRT& ccrt = m_ccrts.emplace_back();
            PASS_IF(only_metadata, ccrt.read(chunk_reader), { m_ccrts.pop_back(); });
        } else if (std::memcmp(fourcc.data(), "CAMG", 4) == 0) {
            CAMG& camg = m_camg.emplace();
            PASS_IF(only_metadata, camg.read(chunk_reader), { m_camg = std::nullopt; });
        } else if (std::memcmp(fourcc.data(), "DYCP", 4) == 0) {
            DYCP& dycp = m_dycp.emplace();
            PASS_IF(only_metadata, dycp.read(chunk_reader), { m_dycp = std::nullopt; });
        } else if (std::memcmp(fourcc.data(), "CTBL", 4) == 0) {
            m_ctbl = std::make_unique<CTBL>();
            PASS_IF(only_metadata, m_ctbl->read(chunk_reader), { m_ctbl = nullptr; });
        } else if (std::memcmp(fourcc.data(), "SHAM", 4) == 0) {
            m_sham = std::make_unique<SHAM>();
            PASS_IF(only_metadata, m_sham->read(chunk_reader), { m_sham = nullptr; });
        } else if (std::memcmp(fourcc.data(), "PCHG", 4) == 0) {
            m_pchg = std::make_unique<PCHG>();
            PASS_IF(only_metadata, m_pchg->read(chunk_reader), { m_pchg = nullptr; });
        } else if (std::memcmp(fourcc.data(), "NAME", 4) == 0) {
            NAME& name = m_name.emplace();
            PASS_IF(only_metadata, name.read(chunk_reader), { m_name = std::nullopt; });
        } else if (std::memcmp(fourcc.data(), "AUTH", 4) == 0) {
            AUTH& auth = m_auth.emplace();
            PASS_IF(only_metadata, auth.read(chunk_reader), { m_auth = std::nullopt; });
        } else if (std::memcmp(fourcc.data(), "ANNO", 4) == 0) {
            ANNO& anno = m_anno.emplace();
            PASS_IF(only_metadata, anno.read(chunk_reader), { m_anno = std::nullopt; });
        } else if (std::memcmp(fourcc.data(), "(c) ", 4) == 0) {
            Copy& copy = m_copy.emplace();
            PASS_IF(only_metadata, copy.read(chunk_reader), { m_copy = std::nullopt; });
        } else {
            // ignore unknown chunk
            // TODO: HAM, SHAM, ...
        }

        chunk_len += chunk_len & 1;
        main_chunk_reader.seek_relative(chunk_len);
    }

    bool laced = false;
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

        if (viewport_mode & CAMG::LACE) {
            laced = true;
        }
    }

    if (m_ctbl || m_sham) {
        auto palette = this->palette();

        if (m_ctbl) {
            auto& palettes = m_ctbl->palettes();

            if (palette) {
                const auto palette_begin = palette->data().begin() + 16;
                const auto palette_end = palette->data().end();
                for (auto& pal : palettes) {
                    std::copy(palette_begin, palette_end, pal.data().begin() + 16);
                }
            }

            if (palettes.size() < (size_t)m_bmhd.height()) {
                LOG_DEBUG(
                    "fewer CTBL palettes than rows in image, extending with zeroed palettes: %zu < %u",
                    palettes.size(), m_bmhd.height());

                if (palette) {
                    palettes.resize((size_t)m_bmhd.height(), *palette);
                } else {
                    palettes.resize((size_t)m_bmhd.height());
                }
            }
        }

        if (m_sham) {
            auto& palettes = m_sham->palettes();

            if (palette) {
                const auto palette_begin = palette->data().begin() + 16;
                const auto palette_end = palette->data().end();
                for (auto& pal : palettes) {
                    std::copy(palette_begin, palette_end, pal.data().begin() + 16);
                }
            }

            size_t height = m_bmhd.height();
            size_t palette_count = laced ? (height + 1) / 2 : height;

            if (palettes.size() < palette_count) {
                LOG_DEBUG(
                    "fewer SHAM palettes than expected, extending with zeroed palettes: %zu < %zu",
                    palettes.size(), palette_count);

                if (palette) {
                    palettes.resize(palette_count, *palette);
                } else {
                    palettes.resize(palette_count);
                }
            }
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
            if (file_type == FileType_PBM) {
                // XXX: why only here and not also in uncompressed?
                line_len = width;
            }
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
            uint8_t v1 = data[offset];
            uint8_t v2 = data[offset + 1];
            uint8_t r = v1 & 0xF;
            uint8_t g = v2 >> 4;
            uint8_t b = v2 & 0xF;

            palette[color_index].assign(
                EXTEND_4_TO_8_BIT(r),
                EXTEND_4_TO_8_BIT(g),
                EXTEND_4_TO_8_BIT(b)
            );
            offset += 2;
        }
    }

    return Result_Ok;
}

Result SHAM::read(MemoryReader& reader) {
    IO(reader.read_u16be(m_version));

    size_t palette_count = reader.remaining() / 32;
    const uint8_t* data = reader.current();
    m_palettes.clear();

    size_t offset = 0;
    for (size_t palette_index = 0; palette_index < palette_count; ++ palette_index) {
        auto& palette = m_palettes.emplace_back();
        for (uint_fast8_t color_index = 0; color_index < 16; ++ color_index) {
            uint8_t v1 = data[offset];
            uint8_t v2 = data[offset + 1];
            uint8_t r = v1 & 0xF;
            uint8_t g = v2 >> 4;
            uint8_t b = v2 & 0xF;

            palette[color_index].assign(
                EXTEND_4_TO_8_BIT(r),
                EXTEND_4_TO_8_BIT(g),
                EXTEND_4_TO_8_BIT(b)
            );
            offset += 2;
        }
    }

    return Result_Ok;
}

Result PCHG::read(MemoryReader& reader) {
    IO(reader.read_u16be(m_compression));
    IO(reader.read_u16be(m_flags));
    IO(reader.read_i16be(m_start_line));
    IO(reader.read_u16be(m_line_count));
    IO(reader.read_u16be(m_changed_lines));
    IO(reader.read_u16be(m_min_reg));
    IO(reader.read_u16be(m_max_reg));
    IO(reader.read_u16be(m_max_changes));
    IO(reader.read_u32be(m_total_changes));

    if (m_max_reg < m_min_reg) {
        LOG_DEBUG("PCHG: Max reg may not be smaller than min reg: %u < %u", m_max_reg, m_min_reg);
        return Result_ParsingError;
    }

    switch (m_compression) {
        case COMP_NONE:
            return this->read_line_data(reader);

        case COMP_HUFFMAN:
        {
            uint32_t comp_info_size;
            uint32_t original_data_size;

            IO(reader.read_u32be(comp_info_size));
            IO(reader.read_u32be(original_data_size));

            std::vector<uint8_t> decompr {};
            decompr.reserve(original_data_size);

            uint32_t index = 0;
            uint32_t bits = 0;
            uint32_t current = 0;
            const uint8_t *startptr = reader.current();
            const uint8_t *endptr = reader.end();
            const uint8_t *tree = startptr + comp_info_size - 2;
            const uint8_t *source = startptr + comp_info_size;
            const uint8_t *ptr = tree;

            while (index < original_data_size) {
                if (!bits) {
                    if (source + 4 > endptr) {
                        LOG_DEBUG("Source buffer overflow while decompressing");
                        return Result_ParsingError;
                    }
                    current = GET_UINT32(source);
                    source += 4;
                    bits = 32;
                }
                if (current & 0x80000000) {
                    int16_t value = (int16_t)GET_UINT16(ptr, 0);
                    if (value >= 0) {
                        decompr.emplace_back((uint8_t)value);
                        ++ index;
                        ptr = tree;
                    } else {
                        ptr += value;

                        if (ptr < startptr) {
                            LOG_DEBUG("Huffman tree underflow while decompressing");
                            return Result_ParsingError;
                        }
                    }
                } else {
                    ptr -= 2;
                    if (ptr < startptr) {
                        LOG_DEBUG("Huffman tree underflow while decompressing");
                        return Result_ParsingError;
                    }
                    int16_t value = (int16_t)GET_UINT16(ptr, 0);
                    if (value > 0 && (value & 0x100)) {
                        decompr.emplace_back((uint8_t)value);
                        ++ index;
                        ptr = tree;
                    }
                }
                current <<= 1;
                -- bits;
            }

            decompr.resize(original_data_size);

            MemoryReader line_reader(decompr.data(), original_data_size);
            return this->read_line_data(line_reader);
        }
        default:
            LOG_DEBUG("PCHG: Unsupported compression value: %d", m_compression);
            return Result_Unsupported;
    }

    return Result_Ok;
}

Result PCHG::read_line_data(MemoryReader& reader) {
    size_t mask_len = (m_line_count + 31) / 32;

    m_line_mask.clear();
    m_line_mask.reserve(mask_len * 4 * 8);

    for (size_t index = 0; index < mask_len; ++ index) {
        uint32_t value;
        IO(reader.read_u32be(value));

        m_line_mask.emplace_back((value & (1 <<  0)) != 0);
        m_line_mask.emplace_back((value & (1 <<  1)) != 0);
        m_line_mask.emplace_back((value & (1 <<  2)) != 0);
        m_line_mask.emplace_back((value & (1 <<  3)) != 0);
        m_line_mask.emplace_back((value & (1 <<  4)) != 0);
        m_line_mask.emplace_back((value & (1 <<  5)) != 0);
        m_line_mask.emplace_back((value & (1 <<  6)) != 0);
        m_line_mask.emplace_back((value & (1 <<  7)) != 0);

        m_line_mask.emplace_back((value & (1 <<  8)) != 0);
        m_line_mask.emplace_back((value & (1 <<  9)) != 0);
        m_line_mask.emplace_back((value & (1 << 10)) != 0);
        m_line_mask.emplace_back((value & (1 << 11)) != 0);
        m_line_mask.emplace_back((value & (1 << 12)) != 0);
        m_line_mask.emplace_back((value & (1 << 13)) != 0);
        m_line_mask.emplace_back((value & (1 << 14)) != 0);
        m_line_mask.emplace_back((value & (1 << 15)) != 0);

        m_line_mask.emplace_back((value & (1 << 16)) != 0);
        m_line_mask.emplace_back((value & (1 << 17)) != 0);
        m_line_mask.emplace_back((value & (1 << 18)) != 0);
        m_line_mask.emplace_back((value & (1 << 19)) != 0);
        m_line_mask.emplace_back((value & (1 << 20)) != 0);
        m_line_mask.emplace_back((value & (1 << 21)) != 0);
        m_line_mask.emplace_back((value & (1 << 22)) != 0);
        m_line_mask.emplace_back((value & (1 << 23)) != 0);

        m_line_mask.emplace_back((value & (1 << 24)) != 0);
        m_line_mask.emplace_back((value & (1 << 25)) != 0);
        m_line_mask.emplace_back((value & (1 << 26)) != 0);
        m_line_mask.emplace_back((value & (1 << 27)) != 0);
        m_line_mask.emplace_back((value & (1 << 28)) != 0);
        m_line_mask.emplace_back((value & (1 << 29)) != 0);
        m_line_mask.emplace_back((value & (1 << 30)) != 0);
        m_line_mask.emplace_back((value & (1 << 31)) != 0);
    }

    // truncate padding bits, if there are any
    m_line_mask.resize(m_line_count, false);

    m_changes.clear();
    m_changes.reserve(m_changed_lines);

    const bool is_small = m_flags & FLAG_12BIT;
    const bool is_big   = m_flags & FLAG_32BIT;

    if (is_small && is_big) {
        LOG_DEBUG("PCHG: Both the 12 bit and 32 bit flag are set!");
        return Result_ParsingError;
    }

    if (!is_small && !is_big) {
        LOG_DEBUG("PCHG: Neither the 12 bit or 32 bit flag is set!");
        return Result_ParsingError;
    }

    for (size_t line_index = 0; line_index < m_changed_lines; ++ line_index) {
        auto& line_changes = m_changes.emplace_back();

        if (is_small) {
            uint8_t change_count16;
            uint8_t change_count32;

            IO(reader.read_u8(change_count16));
            IO(reader.read_u8(change_count32));

            line_changes.reserve((size_t)change_count16 + (size_t)change_count32);

            for (size_t change_index = 0; change_index < change_count16; ++ change_index) {
                uint16_t value;
                IO(reader.read_u16be(value));

                uint8_t reg = value >> 12;
                if (reg >= m_min_reg && reg <= m_max_reg) {
                    uint8_t r = EXTEND_4_TO_8_BIT((value & 0x0F00) >> 8);
                    uint8_t g = EXTEND_4_TO_8_BIT((value & 0x00F0) >> 4);
                    uint8_t b = EXTEND_4_TO_8_BIT((value & 0x000F));

                    line_changes.emplace_back(reg, Color(r, g, b));
                }
            }

            for (size_t change_index = 0; change_index < change_count32; ++ change_index) {
                uint16_t value;
                IO(reader.read_u16be(value));

                uint8_t reg = (value >> 12) + 16;
                if (reg >= m_min_reg && reg <= m_max_reg) {
                    uint8_t r = EXTEND_4_TO_8_BIT((value & 0x0F00) >> 8);
                    uint8_t g = EXTEND_4_TO_8_BIT((value & 0x00F0) >> 4);
                    uint8_t b = EXTEND_4_TO_8_BIT((value & 0x000F));

                    line_changes.emplace_back(reg, Color(r, g, b));
                }
            }
        } else {
            uint32_t change_count;

            IO(reader.read_u32be(change_count));

            line_changes.reserve(change_count);

            for (size_t change_index = 0; change_index < change_count; ++ change_index) {
                uint16_t reg;

                IO(reader.read_u16be(reg));

                uint8_t r, g, b, a;

                // yes, spec says ARBG, not ARGB
                IO(reader.read_u8(a));
                IO(reader.read_u8(r));
                IO(reader.read_u8(b));
                IO(reader.read_u8(g));

                if (reg >= m_min_reg && reg <= m_max_reg) {
                    if (reg > 255) {
                        LOG_DEBUG("PCHG: Palettes bigger than 256 colors aren't supported, color change was for register %u", reg);
                        return Result_Unsupported;
                    }

                    line_changes.emplace_back(reg, Color(r, g, b));
                }
            }
        }
    }

    return Result_Ok;
}

void PCHG::print(std::FILE* file) const {
    fprintf(file,
        "PCHG {\n"
        "    compression: %u,\n"
        "    flags: %u,\n"
        "    start_line: %d,\n"
        "    line_count: %u,\n"
        "    changed_lines: %u,\n"
        "    min_reg: %u,\n"
        "    max_reg: %u,\n"
        "    max_changes: %u,\n"
        "    total_changes: %u,\n"
        "}",
        (unsigned int)m_compression,
        (unsigned int)m_flags,
        (int)m_start_line,
        (unsigned int)m_line_count,
        (unsigned int)m_changed_lines,
        (unsigned int)m_min_reg,
        (unsigned int)m_max_reg,
        (unsigned int)m_max_changes,
        (unsigned int)m_total_changes
    );
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

    auto& bmhd = m_image.bmhd();
    auto num_planes = bmhd.num_planes();
    const auto* camg = m_image.camg();
    m_ham = camg && (camg->viewport_mode() & CAMG::HAM) && (num_planes >= 4 && num_planes <= 8);

    if (!m_image.body() && m_palette) {
        // No image, only a palette: It's a palette file, so draw that palette.
        auto& body = m_image.make_body();

        const uint16_t margin = 1;
        const uint16_t inner_square_size = 4;
        const uint16_t outer_square_size = inner_square_size + margin;

        const uint16_t width  = outer_square_size * 16 + margin;
        const uint16_t height = outer_square_size * 16 + margin;

        bmhd.set_compression(0);
        bmhd.set_mask(0);
        bmhd.set_num_planes(8);
        bmhd.set_x_aspect(1);
        bmhd.set_y_aspect(1);
        bmhd.set_x_origin(0);
        bmhd.set_y_origin(0);
        bmhd.set_width(width);
        bmhd.set_height(height);
        bmhd.set_page_width(width);
        bmhd.set_page_height(height);
        bmhd.set_trans_color(255);

        auto& pixels = body.data();
        pixels.resize((size_t)width * (size_t)height, 255);

        for (uint_fast16_t index = 0; index < 256; ++ index) {
            const uint_fast16_t row = index / 16;
            const uint_fast16_t col = index % 16;
            const size_t x1 = (size_t)col * outer_square_size + margin;
            const size_t y1 = (size_t)row * outer_square_size + margin;
            const size_t x2 = x1 + inner_square_size;
            const size_t y2 = y1 + inner_square_size;

            for (size_t y = y1; y < y2; ++ y) {
                size_t offset = y * (size_t)width;
                for (size_t x = x1; x < x2; ++ x) {
                    pixels[offset + x] = index;
                }
            }
        }
    }

    return result;
}

void Renderer::render(uint8_t* pixels, size_t pitch, double now, bool blend) {
    const auto& header = m_image.bmhd();
    const auto width = header.width();
    const auto height = header.height();

    const auto num_planes = header.num_planes();

    const auto* body = m_image.body();
    const auto& data = body->data();
    const auto& mask = body->mask();
    const bool is_masked = header.mask() == 1;

    const uint8_t* ilbm_pixels = data.data();
    const auto* ctbl = m_image.ctbl();
    const auto* sham = m_image.sham();
    const auto* pchg = m_image.pchg();

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
    } else if (pchg) {
        if (m_palette) {
            m_cycled_palette = *m_palette;
        }

        size_t pixel_len = 3 + is_masked;
        const auto& line_mask = pchg->line_mask();
        const auto& changes = pchg->changes();
        int16_t start_line = pchg->start_line();

        size_t out_line_index = 0;
        size_t ilbm_index = 0;

        // XXX: there is a bug somewhere
        size_t change_index = 0;
        for (int32_t line_index = start_line; line_index < 0; ++ line_index) {
            int32_t mask_index = line_index - start_line;

            if (mask_index >= 0 && (size_t)mask_index < line_mask.size() && line_mask[mask_index]) {
                for (const auto& change : changes[change_index]) {
                    m_cycled_palette[change.reg()] = change.color();
                }
                ++ change_index;
            }
        }

        for (uint16_t y = 0; y < height; ++ y) {
            size_t out_index = out_line_index;

            int32_t mask_index = (int32_t)y - start_line + 1;

            if (mask_index >= 0 && (size_t)mask_index < line_mask.size() && line_mask[mask_index]) {
                for (const auto& change : changes[change_index]) {
                    m_cycled_palette[change.reg()] = change.color();
                }
                ++ change_index;
            }

            for (uint16_t x = 0; x < width; ++ x) {
                auto color = m_cycled_palette[data[ilbm_index]];

                pixels[out_index] = color.r();
                pixels[out_index + 1] = color.g();
                pixels[out_index + 2] = color.b();

                ++ ilbm_index;
                out_index += pixel_len;
            }
            out_line_index += pitch;
        }
    } else if (m_palette || ctbl || sham) {
        if (m_palette) {
            m_cycled_palette.apply_cycles_from(*m_palette, m_cycles, now, blend);
        }
        size_t notlaced = 1;

        // XXX: are SHAM/CTBL palettes cycled?
        const std::vector<Palette> *palettes = nullptr;
        if (ctbl) {
            palettes = &ctbl->palettes();
        } else if (sham) {
            const auto* camg = m_image.camg();
            auto viewport_mode = camg ? camg->viewport_mode() : 0;
            bool laced = viewport_mode & CAMG::LACE;
            notlaced = !laced;
            palettes = &sham->palettes();
        }

        size_t pixel_len = 3 + is_masked;
        size_t palette_index = 0;

        const Palette *palette = &m_cycled_palette;

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

            // TODO: different numbers of num_planes are encoded differently?
            // See: https://en.wikipedia.org/wiki/Hold-And-Modify
            for (uint16_t y = 0; y < height; ++ y) {
                size_t out_index = out_line_index;
                uint8_t r = 0;
                uint8_t g = 0;
                uint8_t b = 0;

                if (palettes) {
                    // XXX: do SHAM palettes need to be cycled?
                    palette = &(*palettes)[palette_index];
                    palette_index += notlaced | (y & 1);
                }

                for (uint16_t x = 0; x < width; ++ x) {
                    uint8_t code = data[ilbm_index];
                    uint8_t mode = code >> payload_bits;
                    uint8_t color_index = code & payload_mask;

                    switch (mode) {
                        case 0:
                        {
                            auto color = (*palette)[color_index];
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
        } else {
            // TODO: Is CTBL/SHAM to be used if HAM flag isn't set?
            size_t out_line_index = 0;
            size_t ilbm_index = 0;

            for (uint16_t y = 0; y < height; ++ y) {
                size_t out_index = out_line_index;

                if (palettes) {
                    // XXX: do SHAM palettes need to be cycled?
                    palette = &(*palettes)[palette_index];
                    palette_index += notlaced | (y & 1);
                }

                for (uint16_t x = 0; x < width; ++ x) {
                    auto color = (*palette)[data[ilbm_index]];
    
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

Result TextChunk::read(MemoryReader& reader) {
    m_content.assign((const char*)reader.current(), reader.remaining());

    return Result_Ok;
}

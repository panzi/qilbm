#include "ILBM.h"
#include "Debug.h"
#include "Try.h"
#include <cstring>
#include <cassert>

#define GET_UINT16(BUF, INDEX) (((uint16_t)((BUF)[(INDEX)]) << 8) | (uint16_t)((BUF)[(INDEX) + 1]))

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
        DEBUG_LOG("truncated BMHD chunk: %zu < %u", reader.remaining(), BMHD::SIZE);
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
        return false;
    }

    if (std::memcmp(fourcc.data(), "FORM", 4) != 0) {
        return false;
    }

    uint32_t main_chuck_len = 0;
    if (!reader.read_u32be(main_chuck_len)) {
        return false;
    }

    if (!reader.read_fourcc(fourcc)) {
        return false;
    }

    if (std::memcmp(fourcc.data(), "ILBM", 4) != 0 && std::memcmp(fourcc.data(), "PBM ", 4) != 0) {
        return false;
    }

    return true;
}

Result ILBM::read(MemoryReader& reader) {
    std::array<char, 4> fourcc;
    IO(reader.read_fourcc(fourcc));

    if (std::memcmp(fourcc.data(), "FORM", 4) != 0) {
        DEBUG_LOG("illegal FOURCC: [0x%02x, 0x%02x, 0x%02x, 0x%02x] \"%c%c%c%c\"",
            fourcc[0], fourcc[1], fourcc[2], fourcc[3],
            fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
        return Result_Unsupported;
    }

    uint32_t main_chuk_len = 0;
    IO(reader.read_u32be(main_chuk_len));
    IO(reader.read_fourcc(fourcc));

    if (std::memcmp(fourcc.data(), "ILBM", 4) == 0) {
        m_file_type = FileType_ILBM;
    } else if (std::memcmp(fourcc.data(), "PBM ", 4) == 0) {
        m_file_type = FileType_PBM;
    } else {
        DEBUG_LOG("unsupported file format: [0x%02x, 0x%02x, 0x%02x, 0x%02x] \"%c%c%c%c\"",
            fourcc[0], fourcc[1], fourcc[2], fourcc[3],
            fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
        return Result_Unsupported;
    }

    m_body = nullptr;
    m_cmap = nullptr;
    m_crngs.clear();
    m_ccrts.clear();

    MemoryReader main_chunk_reader { reader, main_chuk_len };
    while (main_chunk_reader.remaining() > 0) {
        IO(main_chunk_reader.read_fourcc(fourcc));
        uint32_t chunk_len = 0;
        IO(main_chunk_reader.read_u32be(chunk_len));
        MemoryReader chunk_reader { main_chunk_reader, chunk_len };

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
        } else {
            // ignore unknown chunk
            // TODO: HAM, SHAM, ...
        }

        chunk_len += chunk_len & 1;
        main_chunk_reader.seek_relative(chunk_len);
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
    if (reader.remaining()< CAMG::SIZE) {
        DEBUG_LOG("truncated CAMG chunk: %zu < %u", reader.remaining(), CAMG::SIZE);
        return Result_ParsingError;
    }

    IO(reader.read_u32be(m_viewport_mode));

    return Result_Ok;
}

Result CRNG::read(MemoryReader& reader) {
    if (reader.remaining()< CRNG::SIZE) {
        DEBUG_LOG("truncated CRNG chunk: %zu < %u", reader.remaining(), CRNG::SIZE);
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

Result CCRT::read(MemoryReader& reader) {
    if (reader.remaining()< CCRT::SIZE) {
        DEBUG_LOG("truncated CCRT chunk: %zu < %u", reader.remaining(), CCRT::SIZE);
        return Result_ParsingError;
    }

    IO(reader.read_i16be(m_direction));
    if (m_direction < -1 || m_direction > 1) {
        DEBUG_LOG("invalid CCRT direction: %d", m_direction);
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
                DEBUG_LOG("unsupported number of bit planes: %zu", num_planes);
                return Result_Unsupported;
            }
    }
    const size_t width = header.width();
    const size_t height = header.height();

    const size_t plane_len = (width + 15) / 16 * 2;
    size_t line_len = num_planes * plane_len;
    if (header.mask() == 1) {
        line_len += plane_len;
    }
    std::vector<uint8_t> line {};
    line.resize(line_len, 0);

    const size_t data_len = height * line_len;
    const size_t pixel_byte_len = width * height * ((num_planes + 7) / 8);

    m_data.reserve(pixel_byte_len);
    if (header.mask() == 1) {
        m_mask.reserve(width * height);
    }

    switch (header.compression()) {
        case 0:
            // uncompressed
            if (data_len > reader.remaining()) {
                DEBUG_LOG("truncated BODY chunk: %zu < %zu", reader.remaining(), data_len);
                return Result_ParsingError;
            }

            for (uint16_t y = 0; y < header.height(); ++ y) {
                IO(reader.read(line));
                decode_line(line, header.width(), header.mask(), plane_len, num_planes, file_type);
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
                            DEBUG_LOG("broken BODY compression, more data than fits into row: %zu > %zu", next_pos, line_len);
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
                            DEBUG_LOG("broken BODY compression, more data than fits into row: %zu > %zu", next_pos, line_len);
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
                    DEBUG_LOG("expected \"VDAT\" chunk but got: [0x%02x, 0x%02x, 0x%02x, 0x%02x] \"%c%c%c%c\"",
                        fourcc[0], fourcc[1], fourcc[2], fourcc[3],
                        fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
                    return Result_ParsingError;
                }

                uint32_t sub_chunk_len = 0;
                IO(reader.read_u32be(sub_chunk_len));
                if (sub_chunk_len > reader.remaining()) {
                    DEBUG_LOG("truncated compressed BODY chunk: %zu < %zu", reader.remaining(), (size_t)sub_chunk_len);
                    return Result_ParsingError;
                }

                MemoryReader sub_reader { reader, (size_t)sub_chunk_len };
                uint16_t cmd_cnt = 0;
                IO(sub_reader.read_u16be(cmd_cnt));

                if (cmd_cnt < 2) {
                    DEBUG_LOG("error in VDAT, cmd_cnt < 2: %zu", (size_t)cmd_cnt);
                    return Result_ParsingError;
                }
                size_t data_offset = cmd_cnt - 2;
                const uint8_t *buf = sub_reader.data() + sub_reader.offset();
                const size_t buf_len = sub_reader.remaining();
                if (cmd_cnt > buf_len) {
                    DEBUG_LOG("truncated compressed BODY chunk: %zu < %zu", buf_len, (size_t)cmd_cnt);
                    return Result_ParsingError;
                }

                decompr.clear();
                for (size_t cmd_index = 0; cmd_index < cmd_cnt; ++ cmd_index) {
                    int8_t cmd = buf[cmd_index];

                    if (cmd == 0) { // load count from data, COPY
                        size_t next_offset = data_offset + 2;
                        if (next_offset > buf_len || next_offset < data_offset) {
                            DEBUG_LOG("truncated compressed BODY chunk: %zu < %zu", buf_len, next_offset);
                            return Result_ParsingError;
                        }
                        size_t count = GET_UINT16(buf, data_offset);

                        data_offset = next_offset;
                        next_offset += count * 2;
                        if (next_offset < data_offset) {
                            DEBUG_LOG("overflow while decompressing BODY chunk: %zu < %zu", next_offset, data_offset);
                            return Result_ParsingError;
                        }
                        if (next_offset > buf_len) {
                            DEBUG_LOG("truncated compressed BODY chunk: %zu < %zu", buf_len, next_offset);
                            return Result_ParsingError;
                        }
                        std::copy(buf + data_offset, buf + next_offset, std::back_inserter(decompr));
                        data_offset = next_offset;
                    } else if (cmd == 1) { // load count from data, RLE
                        size_t next_offset = data_offset + 2;
                        if (next_offset > buf_len || next_offset < data_offset) {
                            DEBUG_LOG("truncated compressed BODY chunk: %zu < %zu", buf_len, next_offset);
                            return Result_ParsingError;
                        }
                        uint_fast16_t count = GET_UINT16(buf, data_offset);

                        data_offset = next_offset;
                        next_offset += 2;
                        if (next_offset > buf_len || next_offset < data_offset) {
                            DEBUG_LOG("truncated compressed BODY chunk: %zu < %zu", buf_len, next_offset);
                            return Result_ParsingError;
                        }

                        for (uint_fast16_t rle_index = 0; rle_index < count; ++ rle_index) {
                            decompr.emplace_back(buf[data_offset]);
                            decompr.emplace_back(buf[data_offset + 1]);
                        }
                        data_offset = next_offset;
                    } else if (cmd < 0) { // count = -cmd, COPY
                        size_t count = -(int_fast32_t)cmd;
                        size_t next_offset = data_offset + count;
                        if (next_offset < data_offset) {
                            DEBUG_LOG("overflow while decompressing BODY chunk: %zu < %zu", next_offset, data_offset);
                            return Result_ParsingError;
                        }
                        if (next_offset > buf_len) {
                            DEBUG_LOG("truncated compressed BODY chunk: %zu < %zu", buf_len, next_offset);
                            return Result_ParsingError;
                        }
                        std::copy(buf + data_offset, buf + next_offset, std::back_inserter(decompr));
                        data_offset = next_offset;
                    } else { // cmd > 1: count = cmd, RLE
                        size_t count = cmd;
                        size_t next_offset = data_offset + 2;
                        if (next_offset < data_offset) {
                            DEBUG_LOG("overflow while decompressing BODY chunk: %zu < %zu", next_offset, data_offset);
                            return Result_ParsingError;
                        }
                        if (next_offset > buf_len) {
                            DEBUG_LOG("truncated compressed BODY chunk: %zu < %zu", buf_len, next_offset);
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
            }
            break;
        }
        default:
            DEBUG_LOG("unsupported compression flags: %u", header.compression());
            return Result_Unsupported;
    }

    return Result_Ok;
}

void BODY::decode_line(const std::vector<uint8_t>& line, uint8_t mask, uint16_t width, size_t plane_len, size_t num_planes, FileType file_type) {
    switch (file_type) {
        case FileType_ILBM:
            if (num_planes == 24 || num_planes == 32) {
                // TODO: test!
                uint_fast16_t channels = num_planes / 8;
                for (uint_fast16_t x = 0; x < width; ++ x) {
                    auto column = x * channels;
                    for (uint_fast16_t channel = 0; channel < channels; ++ channel) {
                        auto index = column + channel;
                        auto byte_offset = index / 8;
                        auto bit_offset = index % 8;
                        uint8_t value = 0;
                        for (uint_fast16_t plane_index = 0; plane_index < 8; ++ plane_index) {
                            size_t byte_index = plane_len * plane_index + byte_offset;
                            uint8_t bit = (line[byte_index] >> (7 - bit_offset)) & 1;
                            value |= bit << plane_index;
                        }
                        m_data.emplace_back(value);
                    }
                }
            } else {
                for (uint_fast16_t x = 0; x < width; ++ x) {
                    size_t byte_offset = x / 8;
                    size_t bit_offset = x % 8;
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
            DEBUG_LOG("illegal file type value: %d", file_type);
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

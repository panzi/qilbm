#include "ILBM.h"
#include "Debug.h"
#include "Try.h"
#include <cstring>

namespace qilbm {

Result BMHD::read(MemoryReader& reader) {
    if (reader.remaining() < BMHD::SIZE) {
        DEBUG_LOG("truncated BMHD chunk: %zu < %zu", reader.remaining(), BMHD::SIZE);
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
        m_file_type = FileType_BPM;
    } else {
        DEBUG_LOG("unsupported file format: [0x%02x, 0x%02x, 0x%02x, 0x%02x] \"%c%c%c%c\"",
            fourcc[0], fourcc[1], fourcc[2], fourcc[3],
            fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
        return Result_Unsupported;
    }

    m_cmaps.clear();
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
            BODY body;
            TRY(body.read(chunk_reader, m_file_type, m_header));
            m_body = std::optional{ std::move(body) };
        } else if (std::memcmp(fourcc.data(), "CMAP", 4) == 0) {
            CMAP cmap;
            TRY(cmap.read(chunk_reader));
            m_cmaps.emplace_back(std::move(cmap));
        } else if (std::memcmp(fourcc.data(), "CRNG", 4) == 0) {
            CRNG crng;
            TRY(crng.read(chunk_reader));
            m_crngs.emplace_back(std::move(crng));
        } else if (std::memcmp(fourcc.data(), "CCRT", 4) == 0) {
            CCRT ccrt;
            TRY(ccrt.read(chunk_reader));
            m_ccrts.emplace_back(std::move(ccrt));
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
        m_colors.emplace_back(Color(rgb[0], rgb[1], rgb[2]));
    }

    return Result_Ok;
}

Result CAMG::read(MemoryReader& reader) {
    if (reader.remaining()< CAMG::SIZE) {
        DEBUG_LOG("truncated CAMG chunk: %zu < %zu", reader.remaining(), CAMG::SIZE);
        return Result_ParsingError;
    }

    IO(reader.read_u32be(m_viewport_mode));

    return Result_Ok;
}

Result CRNG::read(MemoryReader& reader) {
    if (reader.remaining()< CRNG::SIZE) {
        DEBUG_LOG("truncated CRNG chunk: %zu < %zu", reader.remaining(), CRNG::SIZE);
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
        DEBUG_LOG("truncated CCRT chunk: %zu < %zu", reader.remaining(), CCRT::SIZE);
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

Result BODY::read(MemoryReader& reader, FileType file_type, const BMHD& bmhd) {
    // TODO
    (void)reader;
    (void)file_type;
    (void)bmhd;
    DEBUG_LOG("not implemented");
    return Result_Unsupported;
}

}

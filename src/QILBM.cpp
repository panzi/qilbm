#include "QILBM.h"

#include <climits>

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

QDebug& operator<<(QDebug& debug, const qilbm::CRNG& crng) {
    bool spaces = debug.autoInsertSpaces();
    debug.nospace() << "CRNG { rate: " << crng.rate()
        << ", flags: " << crng.flags()
        << ", low: " << crng.low()
        << ", high: " << crng.high()
        << " }";
    debug.setAutoInsertSpaces(spaces);
    return debug;
}

QDebug& operator<<(QDebug& debug, const qilbm::CCRT& ccrt) {
    bool spaces = debug.autoInsertSpaces();
    debug.nospace() << "CCRT { direction: " << ccrt.direction()
        << ", low: " << ccrt.low()
        << ", high: " << ccrt.high()
        << ", delay_sec: " << ccrt.delay_sec()
        << ", delay_usec: " << ccrt.delay_usec()
        << " }";
    debug.setAutoInsertSpaces(spaces);
    return debug;
}

QDebug& operator<<(QDebug& debug, const qilbm::Cycle& cycle) {
    bool spaces = debug.autoInsertSpaces();
    debug.nospace() << "Cycle { low: " << cycle.low()
        << ", high: " << cycle.high()
        << ", rate: " << cycle.rate()
        << ", reverse: " << cycle.reverse()
        << " }";
    debug.setAutoInsertSpaces(spaces);
    return debug;
}

using namespace qilbm;

const uint qilbm::DEFAULT_FPS = 60;

void ILBMPlugin::readEnvVars() {
    auto env_fps = QString::fromLocal8Bit(qgetenv("QILBM_FPS"));
    bool ok = true;
    uint fps = env_fps.isEmpty() ? DEFAULT_FPS :
        env_fps.toUInt(&ok);
    if (!ok || fps < 1) {
        qDebug() << "ILBMPlugin::readEnvVars(): illegal value for QILBM_FPS environment variable: " << env_fps;
    } else {
        m_fps = fps;
    }

    auto env_blend = QString::fromLocal8Bit(qgetenv("QILBM_BLEND"));
    ok = true;
    bool blend = env_blend.isEmpty() ? false :
        env_blend.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0 ||
        env_blend == QStringLiteral("1");
    if (!ok) {
        qDebug() << "ILBMPlugin::readEnvVars(): illegal value for QILBM_BLEND environment variable: " << env_blend;
    } else {
        m_blend = blend;
    }
}

QImageIOPlugin::Capabilities ILBMPlugin::capabilities(QIODevice *device, const QByteArray &format) const {
    if (format.isNull() && !device) {
        return QImageIOPlugin::Capabilities();
    }

    if (!format.isNull() &&
        format.compare("ilbm", Qt::CaseInsensitive) != 0 &&
        format.compare("lbm", Qt::CaseInsensitive) != 0 &&
        format.compare("iff", Qt::CaseInsensitive) != 0
    ) {
        return QImageIOPlugin::Capabilities();
    }

    if (device && !ILBMHandler::canRead(device)) {
        return QImageIOPlugin::Capabilities();
    }

    return CanRead;
}

ILBMHandler* ILBMPlugin::create(QIODevice *device, const QByteArray &format) const {
    auto handler = new ILBMHandler(m_blend, m_fps);
    handler->setDevice(device);
    if (format.isNull()) {
        handler->setFormat("ilbm");
    } else {
        handler->setFormat(format);
    }

    return handler;
}

ILBMHandler::~ILBMHandler() {}

bool ILBMHandler::canRead() const {
    return m_status == Ok || (m_status == Init && canRead(device()));
}

bool ILBMHandler::canRead(QIODevice *device) {
    if (device == nullptr) {
        qDebug() << "ILBMHandler::canRead(QIODevice*): device is null";
        return false;
    }
    auto data = device->peek(12);
    MemoryReader reader { (const uint8_t*)data.data(), (size_t)data.size() };
    return ILBM::can_read(reader);
}

bool ILBMHandler::read() {
    auto* device = this->device();
    if (device == nullptr) {
        qDebug() << "ILBMHandler::read(): device is null";
        return false;
    }

    m_image = std::make_unique<ILBM>();
    m_palette = nullptr;
    m_cycles.clear();

    auto data = device->readAll();
    MemoryReader reader { (const uint8_t*)data.data(), (size_t)data.size() };
    Result result = m_image->read(reader);

    switch (result) {
        case Result_Ok:
            m_status = Ok;
            break;

        case Result_IOError:
            m_status = IOError;
            return false;

        case Result_ParsingError:
            m_status = ParsingError;
            return false;

        case Result_Unsupported:
            m_status = Unsupported;
            return false;

        default:
            qDebug() << "ILBMHandler::read(): illegal result value: " << result;
            m_status = Unsupported;
            return false;
    }

    if (m_image->body() == nullptr) {
        qDebug() << "ILBMHandler::read(): missing body";
        m_status = NoBody;
        return false;
    }

    for (auto& crng : m_image->crngs()) {
        auto low = crng.low();
        auto high = crng.high();
        auto rate = crng.rate();
        if (low < high && rate > 0) {
            auto flags = crng.flags();
            if ((flags & 1) != 0) {
                if (flags > 3) {
                    qWarning() << "ILBMHandler::read(): Unsupported CRNG flags:" << crng;
                }
                m_cycles.emplace_back(
                    low, high, rate, (flags & 2) != 0
                );
            } else if (flags != 0) {
                qWarning() << "ILBMHandler::read(): Unsupported CRNG flags:" << crng;
            }
        }
    }

    for (auto& ccrt : m_image->ccrts()) {
        auto low = ccrt.low();
        auto high = ccrt.high();
        auto direction = ccrt.direction();
        if (direction != 0 && low < high) {
            uint64_t usec = (uint64_t)ccrt.delay_sec() * 1000000 + (uint64_t)ccrt.delay_usec();
            uint64_t rate = usec * 8903 / 1000000;

            if (direction < -1 || direction > 1) {
                qWarning() << "ILBMHandler::read(): Unsupported CCRT direction:" << ccrt;
            }

            if (rate > 0) {
                m_cycles.emplace_back(
                    low, high, rate, direction > 0
                );
            }
        }
    }

    m_currentFrame = 0;
    m_imageCount = 1;
    const auto* cmap = m_image->cmap();
    if (cmap != nullptr) {
        m_palette = std::make_unique<Palette>();
        Palette& pallete = *m_palette;
        const auto& colors = cmap->colors();
        size_t min_size = std::min(pallete.size(), colors.size());
        for (size_t index = 0; index < min_size; ++ index) {
            pallete[index] = colors[index];
        }

        if (m_cycles.size() > 0) {
            m_imageCount = 0;
        }
    }

    // qDebug().nospace() << "ILBMHandler::read(): "
    //     << "file_type: " << file_type_name(m_image->file_type())
    //     << ", num_planes: " << m_image->header().num_planes()
    //     << ", compression: " << m_image->header().compression()
    //     << ", mask: " << m_image->header().mask()
    //     << ", trans_color: " << m_image->header().trans_color()
    //     << ", has palette: " << (m_image->cmap() != nullptr ? "true" : "false")
    //     << ", imageCount: " << m_imageCount
    //     << ", is animated: " << option(ImageOption::Animation)
    //     << ", this: " << this;

    return true;
}

QRect ILBMHandler::currentImageRect() const {
    if (!m_image) {
        return QRect();
    }

    auto& header = m_image->header();
    return QRect(0, 0, header.width(), header.height());
}

bool ILBMHandler::jumpToImage(int imageNumber) {
    if (imageNumber < 0) {
        return false;
    }

    if (m_palette && m_cycles.size() > 0) {
        m_currentFrame = imageNumber;
        return true;
    }

    if (imageNumber != 0) {
        return false;
    }

    m_currentFrame = 0;
    return true;
}

bool ILBMHandler::jumpToNextImage() {
    if (m_palette && m_cycles.size() > 0) {
        m_currentFrame ++;
        return true;
    }

    return false;
}

int ILBMHandler::nextImageDelay() const {
    if (m_palette && m_cycles.size() > 0) {
        return 1000 / m_fps;
    }
    return 0;
}

static inline QImage::Format qImageFormat(const BMHD& header) {
    const auto num_planes = header.num_planes();
    return
        num_planes == 32 || header.mask() ? QImage::Format::Format_RGBA8888 :
        QImage::Format::Format_RGB888;
}

QVariant ILBMHandler::option(ImageOption option) const {
    switch (option) {
        case ImageOption::Size:
        {
            if (!m_image) {
                return QVariant();
            }
            auto& header = m_image->header();
            return QSize(header.width(), header.height());
        }
        case ImageOption::Animation:
            return (bool)(m_palette && m_cycles.size() > 0);

        case ImageOption::ImageFormat:
        {
            if (!m_image) {
                return QVariant();
            }
            return qImageFormat(m_image->header());
        }
        default:
            return QVariant();
    }
}

bool ILBMHandler::supportsOption(ImageOption option) const {
    switch (option) {
        case ImageOption::Size:
        case ImageOption::Animation:
        case ImageOption::ImageFormat:
            return true;

        default:
            return false;
    }
}

bool ILBMHandler::read(QImage *image) {
    // qDebug().nospace() << "\nILBMHandler::read(QImage*): status: " << statusMessage()
    //     << ", imageCount: " << m_imageCount
    //     << ", currentFrame: " << m_currentFrame
    //     << ", fps: " << m_fps
    //     << ", blend: " << m_blend
    //     << ", this: " << this;

    if (image == nullptr) {
        qDebug() << "ILBMHandler::read(QImage*): image is null";
        return false;
    }

    bool init = m_status == Init;
    if (init && !read()) {
        qDebug() << "ILBMHandler::read(QImage*): read failed, status:" << statusMessage();
        return false;
    }

    if (m_status != Ok || !m_image) {
        qDebug() << "ILBMHandler::read(QImage*): m_image is null! status:" << statusMessage();
        return false;
    }

    const auto& header = m_image->header();
    const auto width = header.width();
    const auto height = header.height();
    const auto format = qImageFormat(header);

    if (width != image->width() || height != image->height() || format != image->format()) {
        if (!allocateImage(QSize(width, height), format, image)) {
            qDebug() << "ILBMHandler::read(QImage*): error allocating image";
            return false;
        }
    }

    const auto num_planes = header.num_planes();

#if 0
    // This isn't viewable in gwenview anyway, so don't spin the CPU for no reason.
    if (init) {
        QString value;
        switch (m_image->file_type()) {
            case FileType_ILBM:
                value = QLatin1String("ILBM");
                break;

            case FileType_PBM:
                value = QLatin1String("PBM");
                break;

            default:
                value = QString(QLatin1String("Invalid file type: %1")).arg((int)m_image->file_type());
                break;
        }

        image->setText(QLatin1String("Sub-Type"), value);
        image->setText(QLatin1String("Bit Planes"), QString::number(num_planes));

        auto& camg = m_image->camg();
        if (camg) {
            auto viewport_mode = camg->viewport_mode();
            value.clear();

            if (viewport_mode & CAMG::EHB) {
                value.append(QLatin1String("EHB"));
            }

            if (viewport_mode & CAMG::HAM) {
                if (!value.isEmpty()) {
                    value.append(QLatin1String(", "));
                }
                value.append(QLatin1String("HAM"));
            }

            if (!value.isEmpty()) {
                image->setText(QLatin1String("Amiga Viewport Modes"), value);
            }
        }

        if (header.x_aspect() != 0) {
            image->setText(QLatin1String("X-Aspect"), QString::number(header.x_aspect()));
        }

        if (header.y_aspect() != 0) {
            image->setText(QLatin1String("Y-Aspect"), QString::number(header.y_aspect()));
        }

        image->setText(QLatin1String("X-Origin"), QString::number(header.x_origin()));
        image->setText(QLatin1String("Y-Origin"), QString::number(header.y_origin()));

        if (header.page_width() != 0) {
            image->setText(QLatin1String("Page-Width"), QString::number(header.page_width()));
        }

        if (header.page_height() != 0) {
            image->setText(QLatin1String("Page-Height"), QString::number(header.page_height()));
        }

        image->setText(QLatin1String("Compression"), QString::number(header.compression()));

        if (header.flags() != 0) {
            image->setText(QLatin1String("Flags"), QString(QLatin1String("0x%1")).arg(header.flags(), 16));
        }

        switch (header.mask()) {
            case 0:
                break;

            case 1:
                image->setText(QLatin1String("Transparency"), QLatin1String("Mask"));
                break;

            case 2:
                image->setText(QLatin1String("Transparency"), QLatin1String("Transparent Color"));
                image->setText(QLatin1String("Transparent Color"), QString::number(header.trans_color()));
                break;

            case 3:
                image->setText(QLatin1String("Transparency"), QLatin1String("Lasso"));
                break;

            default:
                image->setText(QLatin1String("Transparency"), QString::number(header.mask()));
                break;
        }
    }
#endif

    // TODO: Move the following to ILBM.cpp and write to image->bits() directly.
    //       Maybe always using an alpha channel for simplicit?
    const auto* body = m_image->body();
    const auto& data = body->data();
    const auto& mask = body->mask();
    const bool not_masked = header.mask() != 1;

    if (num_planes == 24) {
        for (auto y = 0; y < height; ++ y) {
            size_t mask_offset = (size_t)y * (size_t)width;
            size_t offset = mask_offset * 3;
            for (auto x = 0; x < width; ++ x) {
                size_t index = offset + x * 3;
                int alpha = (not_masked || mask[mask_offset + x]) * 255;
                image->setPixel(x, y, qRgba(data[index], data[index + 1], data[index + 2], alpha));
            }
        }
    } else if (num_planes == 32) {
        for (auto y = 0; y < height; ++ y) {
            size_t offset = (size_t)y * (size_t)width * 4;
            for (auto x = 0; x < width; ++ x) {
                size_t index = offset + x * 4;
                image->setPixel(x, y, qRgba(data[index], data[index + 1], data[index + 2], data[index + 3]));
            }
        }
    } else if (m_palette) {
        double now = (double)m_currentFrame / (double)m_fps;
        Palette palette;
        palette.apply_cycles_from(*m_palette, m_cycles, now, m_blend);

        auto& camg = m_image->camg();
        // TODO: Does HAM without palettes exist? Is then the palette to be assumed all black?
        if (camg && (camg->viewport_mode() & CAMG::HAM) && (num_planes >= 6 && num_planes <= 8)) {
            // HAM decoding http://www.etwright.org/lwsdk/docs/filefmts/ilbm.html
            const uint8_t payload_bits = num_planes - 2;
            const uint8_t ham_shift = 8 - payload_bits;
            const uint8_t ham_mask = (1 << ham_shift) - 1;
            const uint8_t payload_mask = 0xFF >> ham_shift;
            //const uint8_t *lookup_table = COLOR_LOOKUP_TABLES[payload_bits];

            for (auto y = 0; y < height; ++ y) {
                uint8_t r = 0;
                uint8_t g = 0;
                uint8_t b = 0;

                size_t offset = (size_t)y * (size_t)width;
                for (auto x = 0; x < width; ++ x) {
                    size_t index = offset + x;
                    int alpha = (not_masked || mask[index]) * 255;
                    uint8_t code = data[index];
                    uint8_t mode = code >> payload_bits;
                    uint8_t color_index = code & payload_mask;

                    switch (mode) {
                        case 0:
                        {
                            auto& color = palette[color_index];
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

                    image->setPixel(x, y, qRgba(r, g, b, alpha));
                }
            }
        } else {
            for (auto y = 0; y < height; ++ y) {
                size_t offset = (size_t)y * (size_t)width;
                for (auto x = 0; x < width; ++ x) {
                    size_t index = offset + x;
                    int alpha = (not_masked || mask[index]) * 255;
                    auto& color = palette[data[index]];
                    image->setPixel(x, y, qRgba(color.r(), color.g(), color.b(), alpha));
                }
            }
        }

        if (m_cycles.size() > 0) {
            ++ m_currentFrame;
        }
    } else if (num_planes < 8) {
        // XXX: No idea if colors here should be done like in HAM? Need example files.
        // const uint8_t color_shift = 8 - num_planes;
        // const uint8_t color_mask = (1 << color_shift) - 1;

        const uint8_t *lookup_table = COLOR_LOOKUP_TABLES[num_planes];
        for (auto y = 0; y < height; ++ y) {
            size_t offset = (size_t)y * (size_t)width;
            for (auto x = 0; x < width; ++ x) {
                size_t index = offset + x;
                int alpha = (not_masked || mask[index]) * 255;
                uint8_t value = lookup_table[data[index]];
                image->setPixel(x, y, qRgba(value, value, value, alpha));
            }
        }
    } else {
        for (auto y = 0; y < height; ++ y) {
            size_t offset = (size_t)y * (size_t)width;
            for (auto x = 0; x < width; ++ x) {
                size_t index = offset + x;
                int alpha = (not_masked || mask[index]) * 255;
                uint8_t value = data[index];
                image->setPixel(x, y, qRgba(value, value, value, alpha));
            }
        }
    }

    return true;
}

QString ILBMHandler::statusMessage(Status status) {
    switch (status) {
        case Init:
            return QLatin1String("Init");

        case Ok:
            return QLatin1String("Ok");

        case IOError:
            return QLatin1String("IOError");

        case ParsingError:
            return QLatin1String("ParsingError");

        case Unsupported:
            return QLatin1String("Unsupported");

        case NoBody:
            return QLatin1String("NoBody");

        default:
            return QString(QLatin1String("Illegal status code: %1")).arg((int)status);
    }
}

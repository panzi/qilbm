#include "QILBM.h"

#include <climits>

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

    if (m_status == Init && !read()) {
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

    const auto* body = m_image->body();
    const auto& data = body->data();
    const auto& mask = body->mask();
    const auto num_planes = header.num_planes();
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
        if (camg && (camg->viewport_mode() & CAMG::HAM) && (num_planes == 6 || num_planes == 8)) {
            // HAM decoding http://www.etwright.org/lwsdk/docs/filefmts/ilbm.html
            uint8_t code_shift = num_planes - 2;
            uint8_t color_shift = 8 - code_shift;
            uint8_t payload_mask = 0xFF >> color_shift;

            for (auto y = 0; y < height; ++ y) {
                QRgb prev_color = qRgb(0, 0, 0);

                size_t offset = (size_t)y * (size_t)width;
                for (auto x = 0; x < width; ++ x) {
                    size_t index = offset + x;
                    int alpha = (not_masked || mask[index]) * 255;
                    uint8_t code = data[index];
                    uint8_t mode = code >> code_shift;
                    QRgb color;

                    switch (mode) {
                        case 0:
                        {
                            auto& palette_color = palette[code & payload_mask];
                            color = qRgba(palette_color.r(), palette_color.g(), palette_color.b(), alpha);
                            break;
                        }
                        case 1:
                        {
                            // blue
                            uint8_t value = (code & payload_mask) << color_shift;
                            color = qRgba(qRed(prev_color), qGreen(prev_color), value, alpha);
                            break;
                        }
                        case 2:
                        {
                            // red
                            uint8_t value = (code & payload_mask) << color_shift;
                            color = qRgba(value, qGreen(prev_color), qBlue(prev_color), alpha);
                            break;
                        }
                        case 3:
                        {
                            // green
                            uint8_t value = (code & payload_mask) << color_shift;
                            color = qRgba(qRed(prev_color), value, qBlue(prev_color), alpha);
                            break;
                        }
                        default:
                            // not possible
                            color = prev_color;
                            break;
                    }

                    image->setPixel(x, y, color);
                    prev_color = color;
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
    } else {
        uint8_t color_shift = 8 - num_planes;
        for (auto y = 0; y < height; ++ y) {
            size_t offset = (size_t)y * (size_t)width;
            for (auto x = 0; x < width; ++ x) {
                size_t index = offset + x;
                int alpha = (not_masked || mask[index]) * 255;
                uint8_t value = data[index] << color_shift;
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

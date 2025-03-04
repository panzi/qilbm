#include "QILBM.h"

#include <climits>

QDebug& operator<<(QDebug& debug, const qilbm::CRNG& crng) {
    debug << "CRNG { rate:" << crng.rate() <<
        ", flags:" << crng.flags() <<
        ", low:" << crng.low() <<
        ", high:" << crng.high() <<
        " }";
    return debug;
}

QDebug& operator<<(QDebug& debug, const qilbm::CCRT& ccrt) {
    debug << "CCRT { direction:" << ccrt.direction() <<
        ", low:" << ccrt.low() <<
        ", high:" << ccrt.high() <<
        ", delay_sec:" << ccrt.delay_sec() <<
        ", delay_usec:" << ccrt.delay_usec() <<
        " }";
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
        qDebug() << "QILBM: illegal value for QILBM_FPS environment variable: " << env_fps;
    } else {
        m_fps = fps;
    }

    auto env_blend = QString::fromLocal8Bit(qgetenv("QILBM_BLEND"));
    ok = true;
    bool blend = env_blend.isEmpty() ? false :
        env_blend.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0 ||
        env_blend == QStringLiteral("1");
    if (!ok) {
        qDebug() << "QILBM: illegal value for QILBM_BLEND environment variable: " << env_blend;
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
    handler->setFormat(format);

    return handler;
}

ILBMHandler::~ILBMHandler() {}

bool ILBMHandler::canRead(QIODevice *device) {
    if (device == nullptr) {
        qDebug() << "ILBMHandler::canRead: device is null";
        return false;
    }
    auto data = device->peek(12);
    MemoryReader reader { (const uint8_t*)data.data(), (size_t)data.size() };
    return ILBM::can_read(reader);
}

bool ILBMHandler::read() {
    auto* device = this->device();
    if (device == nullptr) {
        qDebug() << "ILBMHandler::read: device is null";
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
            qDebug() << "illegal result value: " << result;
            m_status = Unsupported;
            return false;
    }

    if (m_image->body() == nullptr) {
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
                    qWarning() << "QILBM: Unsupported CRNG flags: " << crng;
                }
                m_cycles.emplace_back(
                    low, high, rate, (flags & 2) != 0
                );
            } else if (flags != 0) {
                qWarning() << "QILBM: Unsupported CRNG flags: " << crng;
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
                qWarning() << "QILBM: Unsupported CCRT direction: " << ccrt;
            }

            if (rate > 0) {
                m_cycles.emplace_back(
                    low, high, rate, direction > 0
                );
            }
        }
    }

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

        for (auto& cycle : m_cycles) {
            int size = (uint)cycle.high() + 1 - (uint)cycle.low();
            // Is this correct?
            int frames = m_fps * size * LBM_CYCLE_RATE_DIVISOR / cycle.rate();
            if (m_imageCount < frames ? frames % m_imageCount != 0 : m_imageCount % frames != 0) {
                m_imageCount = INT_MAX / m_imageCount > frames ? INT_MAX : m_imageCount * frames;
            }
        }
    }

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
        num_planes == 32 ? QImage::Format::Format_RGBA8888 :
        num_planes == 24 && header.mask() ? QImage::Format::Format_RGBA8888 :
        num_planes == 24 ? QImage::Format::Format_RGB888 :
        QImage::Format::Format_Indexed8;
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
    if (image == nullptr) {
        qDebug() << "ILBMHandler::read: image is null";
        return false;
    }

    if (m_status == Init && !read()) {
        qDebug() << "ILBMHandler::read: status:" << statusMessage();
        return false;
    }

    if (!m_image) {
        qDebug() << "ILBMHandler::read: m_image is null! status:" << statusMessage();
        return false;
    }

    bool init = m_currentFrame == 0;
    const auto& header = m_image->header();
    const auto width = header.width();
    const auto height = header.height();
    const auto format = qImageFormat(header);

    if (width != image->width() || height != image->height() || format != image->format()) {
        init = true;
        *image = QImage((int)width, (int)height, format);
        if (image->isNull()) {
            qDebug() << "ILBMHandler::read: error allocating image";
            return false;
        }
    }

    if (init) {
        const auto* body = m_image->body();
        const auto& data = body->data();
        const auto& mask = body->mask();
        const bool is_masked = header.mask() == 1;

        if (header.num_planes() == 24) {
            for (auto y = 0; y < height; ++ y) {
                size_t mask_offset = (size_t)y * (size_t)width;
                size_t offset = mask_offset * 3;
                for (auto x = 0; x < width; ++ x) {
                    size_t index = offset + x * 3;
                    if (is_masked && !mask[mask_offset + x]) {
                        image->setPixel(x, y, qRgba(0, 0, 0, 0));
                    } else {
                        image->setPixel(x, y, qRgb(data[index], data[index + 1], data[index + 2]));
                    }
                }
            }
        } else if (header.num_planes() == 32) {
            for (auto y = 0; y < height; ++ y) {
                size_t offset = (size_t)y * (size_t)width * 4;
                for (auto x = 0; x < width; ++ x) {
                    size_t index = offset + x * 4;
                    image->setPixel(x, y, qRgba(data[index], data[index + 1], data[index + 2], data[index + 3]));
                }
            }
        } else {
            for (auto y = 0; y < height; ++ y) {
                size_t offset = (size_t)y * (size_t)width;
                for (auto x = 0; x < width; ++ x) {
                    size_t index = offset + x;
                    if (is_masked && !mask[index]) {
                        image->setPixel(x, y, 256);
                    } else {
                        image->setPixel(x, y, data[index]);
                    }
                }
            }
        }
    }

    if (m_palette) {
        double now = (double)m_currentFrame / (double)m_fps;
        Palette palette;
        palette.apply_cycles_from(*m_palette, m_cycles, now, m_blend);

        image->setColorCount(257);
        for (size_t index = 0; index < palette.size(); ++ index) {
            auto color = palette[index];
            image->setColor(index, qRgb(color.r(), color.g(), color.b()));
        }
        if (header.mask() >= 2) {
            image->setColor(header.trans_color(), qRgba(0, 0, 0, 0));
        }
        // for mask:
        image->setColor(256, qRgba(0, 0, 0, 0));
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

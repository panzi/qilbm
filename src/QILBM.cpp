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
    auto env_fps = QString::fromLocal8Bit(qgetenv("QILBM_FPS")).trimmed();
    bool ok = true;
    uint fps = env_fps.isEmpty() ? DEFAULT_FPS :
        env_fps.toUInt(&ok);
    if (!ok || fps < 1) {
        qWarning().nospace() << Q_FUNC_INFO << ": illegal value for QILBM_FPS environment variable: " << env_fps;
    } else if (fps > 1000) {
        qWarning().nospace() << Q_FUNC_INFO << ": value of QILBM_FPS environment variable is too big, limited to 1000 fps: " << env_fps;
    } else {
        m_fps = fps;
    }

    auto env_blend = QString::fromLocal8Bit(qgetenv("QILBM_BLEND")).trimmed();
    ok = true;
    bool blend = env_blend.isEmpty() ? false :
        env_blend.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0 ||
        env_blend == QStringLiteral("1");
    if (!ok) {
        qWarning().nospace() << Q_FUNC_INFO << ": illegal value for QILBM_BLEND environment variable: " << env_blend;
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
        qDebug().nospace() << Q_FUNC_INFO << ": device is null";
        return false;
    }
    auto data = device->peek(12);
    MemoryReader reader { (const uint8_t*)data.data(), (size_t)data.size() };
    return ILBM::can_read(reader);
}

bool ILBMHandler::read() {
    auto* device = this->device();
    if (device == nullptr) {
        qDebug().nospace() << Q_FUNC_INFO << ": device is null";
        return false;
    }

    auto data = device->readAll();
    MemoryReader reader { (const uint8_t*)data.data(), (size_t)data.size() };
    Result result = m_renderer.read(reader);

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
            qDebug().nospace() << Q_FUNC_INFO << ": illegal result value: " << result;
            m_status = Unsupported;
            return false;
    }

    if (m_renderer.image().body() == nullptr) {
        qDebug().nospace() << Q_FUNC_INFO << ": missing body";
        m_status = NoBody;
        return false;
    }

    m_currentFrame = 0;
    m_imageCount = m_renderer.is_animated() ? 0 : 1;

    return true;
}

QRect ILBMHandler::currentImageRect() const {
    auto& header = m_renderer.image().bmhd();
    return QRect(0, 0, header.width(), header.height());
}

bool ILBMHandler::jumpToImage(int imageNumber) {
    if (imageNumber < 0) {
        return false;
    }

    if (m_renderer.is_animated()) {
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
    if (m_renderer.is_animated()) {
        m_currentFrame ++;
        return true;
    }

    return false;
}

int ILBMHandler::nextImageDelay() const {
    if (m_renderer.is_animated()) {
        return 1000 / m_fps;
    }
    return 0;
}

static inline QImage::Format qImageFormat(const BMHD& header) {
    const auto num_planes = header.num_planes();
    return
        num_planes == 32 || header.mask() == 1 ? QImage::Format::Format_RGBA8888 :
        QImage::Format::Format_RGB888;
}

QVariant ILBMHandler::option(ImageOption option) const {
    switch (option) {
        case ImageOption::Size:
        {
            const auto& header = m_renderer.image().bmhd();
            return QSize(header.width(), header.height());
        }
        case ImageOption::Animation:
            return m_renderer.is_animated();

        case ImageOption::ImageFormat:
            return qImageFormat(m_renderer.image().bmhd());

        case ImageOption::Name:
        {
            const auto* name = m_renderer.image().name();
            if (name == nullptr) {
                return QVariant();
            }
            return QString::fromLatin1(name->content());
        }
        case ImageOption::Description:
        {
            const auto& image = m_renderer.image();
            const auto* auth = image.auth();
            const auto* copy = image.copy();
            const auto* anno = image.anno();

            if (!auth && !anno && !copy) {
                return QVariant();
            }

            QString value;

            if (auth) {
                value.append(QLatin1StringView(auth->content()));
            }

            if (copy) {
                if (!value.isEmpty()) {
                    value.append(QChar::fromLatin1('\n'));
                }
                value.append(QLatin1StringView(copy->content()));
            }

            if (anno) {
                if (!value.isEmpty()) {
                    value.append(QChar::fromLatin1('\n'));
                }
                value.append(QLatin1StringView(anno->content()));
            }

            return value;
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
        case ImageOption::Name:
        case ImageOption::Description:
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
        qDebug().nospace() << Q_FUNC_INFO << ": image is null";
        return false;
    }

    bool init = m_status == Init;
    if (init && !read()) {
        qDebug().nospace() << Q_FUNC_INFO << ": read failed, status: " << statusMessage();
        return false;
    }

    if (m_status != Ok) {
        qDebug().nospace() << Q_FUNC_INFO << ": status: " << statusMessage();
        return false;
    }

    const auto& header = m_renderer.image().bmhd();
    const auto width = header.width();
    const auto height = header.height();
    const auto format = qImageFormat(header);

    if (width != image->width() || height != image->height() || format != image->format()) {
        if (!allocateImage(QSize(width, height), format, image)) {
            qDebug().nospace() << Q_FUNC_INFO << ": error allocating image";
            return false;
        }
    }

    double now = (double)m_currentFrame / (double)m_fps;
    //qInfo() << "FPS:" << m_fps << "delay:" << (1000 / m_fps) << "ms" << "now:" << now;
    m_renderer.render((uint8_t*)image->bits(), image->bytesPerLine(), now, m_blend);

    if (m_renderer.is_animated()) {
        ++ m_currentFrame;
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

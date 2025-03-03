#include "QILBM.h"

namespace qilbm {

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

QImageIOHandler* ILBMPlugin::create(QIODevice *device, const QByteArray &format) const {
    auto env_fps = QString::fromLocal8Bit(qgetenv("QILBM_FPS"));
    bool ok = true;
    uint fps = env_fps.isEmpty() ? ILBMHandler::DEFAULT_FPS :
        env_fps.toUInt(&ok);
    if (!ok || fps < 1) {
        qDebug() << "QILBM: illegal value for QILBM_FPS environment variable: " << env_fps;
        fps = ILBMHandler::DEFAULT_FPS;
    }

    auto env_blend = QString::fromLocal8Bit(qgetenv("QILBM_BLEND"));
    ok = true;
    bool blend = env_blend.isEmpty() ? false :
        env_blend.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0 ||
        env_blend == QStringLiteral("1");
    if (!ok) {
        qDebug() << "QILBM: illegal value for QILBM_BLEND environment variable: " << env_blend;
    }

    auto handler = new ILBMHandler(blend, fps);
    handler->setDevice(device);
    handler->setFormat(format);

    return handler;
}

const uint ILBMHandler::DEFAULT_FPS = 60;

ILBMHandler::~ILBMHandler() {}

}

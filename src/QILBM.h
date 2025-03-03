#ifndef QILBM_QILBM_H
#define QILBM_QILBM_H
#pragma once

#include <QImageIOPlugin>
#include "ILBM.h"

namespace qilbm {

class ILBMPlugin : public QImageIOPlugin {
    Q_OBJECT
    Q_CLASSINFO("author", "Mathias Panzenböck")
    Q_CLASSINFO("url", "https://github.com/panzi/qilbm")
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QImageIOHandlerFactoryInterface" FILE "ilbm.json")

public:
    ILBMPlugin(QObject *parent = nullptr) : QImageIOPlugin(parent) {}

    Capabilities capabilities(QIODevice *device, const QByteArray &format) const;
    QImageIOHandler* create(QIODevice *device, const QByteArray &format) const;
};

class ILBMHandler : public QImageIOHandler {
private:
    bool m_blend;
    uint m_fps;

    int m_currentFrame;
    ILBM m_image;

public:
    static const uint DEFAULT_FPS;

    ILBMHandler() :
        QImageIOHandler(), m_blend(false), m_fps(DEFAULT_FPS), m_currentFrame(0), m_image() {}

    ILBMHandler(bool blend, uint fps) :
        QImageIOHandler(), m_blend(blend), m_fps(fps), m_currentFrame(0), m_image() {}

    ~ILBMHandler();

    bool canRead() const;
    int currentImageNumber() const { return m_currentFrame; }
    QRect currentImageRect() const;
    int imageCount() const;
    bool jumpToImage(int imageNumber);
    bool jumpToNextImage();
    int loopCount() const;
    int nextImageDelay() const;
    QVariant option(ImageOption option) const;
    bool read(QImage *image);
    bool supportsOption(ImageOption option) const;

    static bool canRead(QIODevice *device);

    bool read();

    bool blend() const { return m_blend; }
    void setBlend(bool blend) { m_blend = blend; }

    uint fps() const { return m_fps; }
    void setFps(uint fps) {
        if (fps > 0) {
            m_fps = fps;
        }
    }
};

}

#endif

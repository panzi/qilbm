#ifndef QILBM_QILBM_H
#define QILBM_QILBM_H
#pragma once

#include <QImageIOPlugin>
#include <memory>
#include <vector>
#include "ILBM.h"
#include "Palette.h"

QDebug& operator<<(QDebug& debug, const qilbm::CRNG& crng);
QDebug& operator<<(QDebug& debug, const qilbm::CCRT& ccrt);
QDebug& operator<<(QDebug& debug, const qilbm::Cycle& cycle);

namespace qilbm {

extern const uint DEFAULT_FPS;

class ILBMHandler : public QImageIOHandler {
public:
    enum Status {
        Init = -1,
        Ok = 0,
        IOError = 1,
        ParsingError = 2,
        Unsupported = 3,
        NoBody = 4,
    };

private:
    Status m_status;
    bool m_blend;
    uint m_fps;
    int m_imageCount;
    int m_currentFrame;
    Renderer m_renderer;

public:
    ILBMHandler(bool blend = false, uint fps = DEFAULT_FPS) :
        QImageIOHandler(), m_status(Init), m_blend(blend), m_fps(fps),
        m_imageCount(0), m_currentFrame(-1), m_renderer() {}

    ~ILBMHandler();

    bool canRead() const override;
    int currentImageNumber() const override { return m_currentFrame; }
    QRect currentImageRect() const override;
    int imageCount() const override { return m_imageCount; }
    bool jumpToImage(int imageNumber) override;
    bool jumpToNextImage() override;
    int loopCount() const override { return INT_MAX; }
    int nextImageDelay() const override;
    QVariant option(ImageOption option) const override;
    bool read(QImage *image) override;
    bool supportsOption(ImageOption option) const override;

    static bool canRead(QIODevice *device);

    bool read();
    inline Status status() const { return m_status; }

    inline bool blend() const { return m_blend; }
    void setBlend(bool blend) { m_blend = blend; }

    inline uint fps() const { return m_fps; }
    void setFps(uint fps) {
        if (fps > 0) {
            m_fps = fps;
        }
    }

    static QString statusMessage(Status status);
    QString statusMessage() const { return statusMessage(m_status); };
};

class ILBMPlugin : public QImageIOPlugin {
    Q_OBJECT
    Q_CLASSINFO("author", "Mathias Panzenböck")
    Q_CLASSINFO("url", "https://github.com/panzi/qilbm")
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QImageIOHandlerFactoryInterface" FILE "ilbm.json")

private:
    bool m_blend;
    uint m_fps;

protected:
    void readEnvVars();

public:
    ILBMPlugin(QObject *parent = nullptr) :
        QImageIOPlugin(parent), m_blend(false), m_fps(DEFAULT_FPS) {
        readEnvVars();
    }

    ILBMPlugin(QObject *parent, bool blend = false, uint fps = DEFAULT_FPS) :
        QImageIOPlugin(parent), m_blend(blend), m_fps(fps == 0 ? 1 : fps) {}

    Capabilities capabilities(QIODevice *device, const QByteArray &format) const override;
    ILBMHandler* create(QIODevice *device, const QByteArray &format) const override;

    inline bool blend() const { return m_blend; }
    void setBlend(bool blend) { m_blend = blend; }

    inline uint fps() const { return m_fps; }
    void setFps(uint fps) {
        if (fps > 0) {
            m_fps = fps;
        }
    }
};

}

#endif

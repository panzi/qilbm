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

    Capabilities capabilities(QIODevice *device, const QByteArray &format) const;
    QImageIOHandler* create(QIODevice *device, const QByteArray &format) const;
};

class ILBMHandler : public QImageIOHandler {

public:
    ILBMHandler();
    ~ILBMHandler();

    bool canRead() const;
    int currentImageNumber() const;
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

private:

    enum State {
        Ready,
        Read,
        Error
    };

    State state;
    int currentFrame;
    ILBM image;
};

}

#endif

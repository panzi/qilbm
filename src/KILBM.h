#ifndef QILBM_KILBM_H
#define QILBM_KILBM_H
#pragma once

#include "ILBM.h"

#include <QtCore/QObject>
#include <KF6/KFileMetaData/kfilemetadata/extractorplugin.h>

namespace qilbm {

class ILBMExtractor : public KFileMetaData::ExtractorPlugin {
    Q_OBJECT
    Q_CLASSINFO("author", "Mathias Panzenböck")
    Q_CLASSINFO("url", "https://github.com/panzi/qilbm")
    Q_PLUGIN_METADATA(IID "org.kde.kf5.kfilemetadata.ExtractorPlugin"
                      FILE "ilbmextractor.json")
    Q_INTERFACES(KFileMetaData::ExtractorPlugin)

public:
    explicit ILBMExtractor(QObject *parent = nullptr);

    void extract(KFileMetaData::ExtractionResult *result) override;
    QStringList mimetypes() const override;
};

}

#endif

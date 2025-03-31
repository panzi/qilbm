#include "KILBM.h"

#include <QtCore/QFile>

using namespace qilbm;

ILBMExtractor::ILBMExtractor(QObject *parent) : ExtractorPlugin(parent) {}

QStringList ILBMExtractor::mimetypes() const
{
    return {
        QStringLiteral("image/x-ilbm"),
        QStringLiteral("image/x-lbm")
    };
}

void ILBMExtractor::extract(KFileMetaData::ExtractionResult* result)
{
    QFile file(result->inputUrl());

    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    auto data = file.readAll();
    ILBM ilbm;

    MemoryReader reader { (const uint8_t*)data.data(), (size_t)data.size() };

    // TODO: don't read body, don't fail on things that can't be read
    auto res = ilbm.read(reader, true);

    if (res != Result_Ok) {
        return;
    }

    const auto& bmhd = ilbm.bmhd();
    const auto* anno = ilbm.anno();
    const auto* auth = ilbm.auth();
    const auto* copy = ilbm.copy();
    const auto* name = ilbm.name();
    const auto* camg = ilbm.camg();
    const auto* ctbl = ilbm.ctbl();
    const auto* sham = ilbm.sham();
    const auto* pchg = ilbm.pchg();

    result->addType(KFileMetaData::Type::Image);

    if (result->inputFlags() & KFileMetaData::ExtractionResult::Flag::ExtractImageData) {
        // TODO
    }

    if (result->inputFlags() & KFileMetaData::ExtractionResult::Flag::ExtractMetaData) {
        result->add(KFileMetaData::Property::Width,  bmhd.width());
        result->add(KFileMetaData::Property::Height, bmhd.height());

        result->append(QStringLiteral("Sub-Type: %1\n").arg(QLatin1String(qilbm::file_type_name(ilbm.file_type()))));
        result->append(QStringLiteral("X-Origin: %1\n").arg((int)bmhd.x_origin()));
        result->append(QStringLiteral("Y-Origin: %1\n").arg((int)bmhd.y_origin()));
        result->append(QStringLiteral("Planes: %1\n").arg((uint)bmhd.num_planes()));
        result->append(QStringLiteral("Mask: %1\n").arg((uint)bmhd.mask()));
        result->append(QStringLiteral("Compression: %1\n").arg((uint)bmhd.compression()));
        result->append(QStringLiteral("Flags: %1\n").arg((uint)bmhd.flags()));
        result->append(QStringLiteral("Transparent Color: %1\n").arg((uint)bmhd.trans_color()));
        result->append(QStringLiteral("X-Aspect: %1\n").arg((uint)bmhd.x_aspect()));
        result->append(QStringLiteral("Y-Aspect: %1\n").arg((uint)bmhd.y_aspect()));
        result->append(QStringLiteral("Page-Width: %1\n").arg((int)bmhd.page_width()));
        result->append(QStringLiteral("Page-Height: %1\n").arg((int)bmhd.page_height()));

        uint32_t viewport_mode = camg ? camg->viewport_mode() : 0;

        if (viewport_mode != 0) {
            QString text;
            text.append(QLatin1String("Viewport-Mode:"));

            if (viewport_mode & qilbm::CAMG::LACE) {
                text.append(QLatin1String(" LACE"));
            }

            if (viewport_mode & qilbm::CAMG::EHB) {
                text.append(QLatin1String(" EHB"));
            }

            if (viewport_mode & qilbm::CAMG::HAM) {
                text.append(QLatin1String(" HAM"));
            }

            if (viewport_mode & qilbm::CAMG::HIRES) {
                text.append(QLatin1String(" HIRES"));
            }
            text.append(QLatin1String("\n"));

            result->append(text);
        }

        if (ctbl) {
            result->append(QStringLiteral("CTBL Palettes: %1\n").arg(ctbl->palettes().size()));
        }

        if (sham) {
            result->append(QStringLiteral("SHAM Version: %1\n").arg((uint)sham->version()));
            result->append(QStringLiteral("SHAM Palettes: %1\n").arg(sham->palettes().size()));
        }

        if (pchg) {
            result->append(QStringLiteral("PCHG Compression: %1\n").arg(
                QLatin1String(pchg->compression() == qilbm::PCHG::COMP_HUFFMAN ? "Huffman" : "None")
            ));

            QString text;
            text.append(QLatin1String("PCHG Flags:"));

            if (pchg->flags() & qilbm::PCHG::FLAG_12BIT) {
                text.append(QLatin1String(" 12BIT"));
            }

            if (pchg->flags() & qilbm::PCHG::FLAG_32BIT) {
                text.append(QLatin1String(" 32BIT"));
            }

            if (pchg->flags() & qilbm::PCHG::FLAG_USE_ALPHA) {
                text.append(QLatin1String(" USE_ALPHA"));
            }
            text.append(QLatin1String("\n"));

            result->append(text);

            result->append(QStringLiteral("PCHG Start Line: %1\n").arg((int)pchg->start_line()));
            result->append(QStringLiteral("PCHG Line Count: %1\n").arg((uint)pchg->line_count()));
            result->append(QStringLiteral("PCHG Changed Lines: %1\n").arg((uint)pchg->changed_lines()));
            result->append(QStringLiteral("PCHG Minimum Register: %1\n").arg((uint)pchg->min_reg()));
            result->append(QStringLiteral("PCHG Maximum Register: %1\n").arg((uint)pchg->max_reg()));
            result->append(QStringLiteral("PCHG Maximum Changes: %1\n").arg((uint)pchg->max_changes()));
            result->append(QStringLiteral("PCHG Total Changes: %1\n").arg((uint)pchg->total_changes()));
        }

        if (anno) {
            result->add(KFileMetaData::Property::Comment, QString::fromStdString(anno->content()));
        }

        if (auth) {
            result->add(KFileMetaData::Property::Author, QString::fromStdString(auth->content()));
        }

        if (copy) {
            result->add(KFileMetaData::Property::Copyright, QString::fromStdString(copy->content()));
        }

        if (name) {
            result->add(KFileMetaData::Property::Title, QString::fromStdString(name->content()));
        }
    }
}

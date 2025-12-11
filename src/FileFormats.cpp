#include "FileFormats.h"
#include <QFile>
#include <QFileInfo>
#include <QDebug>

namespace FileFormats {

Format detectFormat(const QString& filePath)
{
    QFileInfo info(filePath);
    QString ext = info.suffix().toLower();

    if (ext == "tfi") return Format::TFI;
    if (ext == "dmp") return Format::DMP;
    if (ext == "opn") return Format::OPN;
    if (ext == "geb") return Format::GEB;

    return Format::Unknown;
}

std::optional<FMPatch> loadFMPatch(const QString& filePath)
{
    Format format = detectFormat(filePath);

    switch (format) {
        case Format::TFI: return loadTFI(filePath);
        case Format::DMP: return loadDMP(filePath);
        case Format::OPN: return loadOPN(filePath);
        default:
            qWarning() << "Unknown file format:" << filePath;
            return std::nullopt;
    }
}

std::optional<FMPatch> loadTFI(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open TFI file:" << filePath;
        return std::nullopt;
    }

    QByteArray data = file.readAll();
    file.close();

    // TFI is exactly 42 bytes
    if (data.size() != 42) {
        qWarning() << "Invalid TFI file size:" << data.size() << "(expected 42)";
        return std::nullopt;
    }

    FMPatch patch = FMPatch::fromBytes(reinterpret_cast<const uint8_t*>(data.constData()));

    // Use filename as patch name
    QFileInfo info(filePath);
    patch.name = info.baseName();

    qDebug() << "Loaded TFI patch:" << patch.name;
    return patch;
}

std::optional<FMPatch> loadDMP(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open DMP file:" << filePath;
        return std::nullopt;
    }

    QByteArray data = file.readAll();
    file.close();

    if (data.size() < 51) {
        qWarning() << "DMP file too small:" << data.size();
        return std::nullopt;
    }

    const uint8_t* d = reinterpret_cast<const uint8_t*>(data.constData());

    // DMP format header
    uint8_t version = d[0];

    // Check if it's an FM instrument (not sample-based)
    uint8_t system;
    int offset;

    if (version >= 11) {
        // DefleMask 1.0+ format
        system = d[1];
        offset = 2;

        // 1 = Genesis, 2 = Genesis (ext ch3), 8 = Arcade
        if (system != 1 && system != 2 && system != 8) {
            qWarning() << "DMP is not a Genesis/FM instrument (system:" << system << ")";
            return std::nullopt;
        }

        // Check mode byte (1 = FM)
        if (d[offset] != 1) {
            qWarning() << "DMP is not an FM instrument";
            return std::nullopt;
        }
        offset++;
    } else {
        // Older format
        system = 1;  // Assume Genesis
        offset = 1;
    }

    FMPatch patch;

    // Read FM parameters
    // DMP order: LFO, FB, ALG, then 4 operators
    // We skip LFO (not used in our synth)

    if (version >= 11) {
        // Skip LFO
        offset++;
    }

    patch.feedback = d[offset++];
    patch.algorithm = d[offset++];

    // Read 4 operators
    // DMP operator order: 1, 3, 2, 4 (same as TFI)
    for (int op = 0; op < 4; op++) {
        patch.op[op].mul = d[offset++];
        patch.op[op].tl = d[offset++];
        patch.op[op].ar = d[offset++];
        patch.op[op].dr = d[offset++];
        patch.op[op].sl = d[offset++];
        patch.op[op].rr = d[offset++];

        if (version >= 11) {
            // Extended parameters
            patch.op[op].rs = d[offset++];   // AM
            offset++;                         // Skip KSL
            patch.op[op].dt = d[offset++];
            offset++;                         // Skip D2R
            patch.op[op].ssg = d[offset++];
        } else {
            // Older format
            patch.op[op].dt = d[offset++];
            patch.op[op].rs = d[offset++];
            patch.op[op].sr = d[offset++];
            patch.op[op].ssg = d[offset++];
        }
    }

    // Clamp values to valid ranges
    patch.algorithm &= 0x07;
    patch.feedback &= 0x07;
    for (int op = 0; op < 4; op++) {
        patch.op[op].mul &= 0x0F;
        patch.op[op].dt &= 0x07;
        patch.op[op].tl &= 0x7F;
        patch.op[op].rs &= 0x03;
        patch.op[op].ar &= 0x1F;
        patch.op[op].dr &= 0x1F;
        patch.op[op].sr &= 0x1F;
        patch.op[op].rr &= 0x0F;
        patch.op[op].sl &= 0x0F;
        patch.op[op].ssg &= 0x0F;
    }

    // Use filename as patch name
    QFileInfo info(filePath);
    patch.name = info.baseName();

    qDebug() << "Loaded DMP patch:" << patch.name << "(version" << version << ")";
    return patch;
}

std::optional<FMPatch> loadOPN(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open OPN file:" << filePath;
        return std::nullopt;
    }

    QByteArray data = file.readAll();
    file.close();

    // OPN format is similar to TFI but may have header
    // Try different offsets

    FMPatch patch;
    const uint8_t* d = reinterpret_cast<const uint8_t*>(data.constData());

    if (data.size() >= 42) {
        // Try as raw TFI-like format
        patch = FMPatch::fromBytes(d);
    } else if (data.size() >= 51) {
        // OPN with header (skip first 9 bytes)
        patch = FMPatch::fromBytes(d + 9);
    } else {
        qWarning() << "OPN file too small:" << data.size();
        return std::nullopt;
    }

    // Validate values
    if (patch.algorithm > 7 || patch.feedback > 7) {
        qWarning() << "Invalid OPN data";
        return std::nullopt;
    }

    QFileInfo info(filePath);
    patch.name = info.baseName();

    qDebug() << "Loaded OPN patch:" << patch.name;
    return patch;
}

bool saveTFI(const QString& filePath, const FMPatch& patch)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }

    auto bytes = patch.toBytes();
    file.write(reinterpret_cast<const char*>(bytes.data()), 42);
    file.close();

    qDebug() << "Saved TFI patch:" << filePath;
    return true;
}

QString loadFilterString()
{
    return QObject::tr(
        "All Patch Files (*.tfi *.dmp *.opn);;"
        "TFI Files (*.tfi);;"
        "DefleMask Patches (*.dmp);;"
        "OPN Patches (*.opn);;"
        "All Files (*)"
    );
}

QString saveFilterString()
{
    return QObject::tr(
        "TFI Files (*.tfi);;"
        "All Files (*)"
    );
}

}  // namespace FileFormats

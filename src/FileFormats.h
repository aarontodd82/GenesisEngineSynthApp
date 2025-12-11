#ifndef FILEFORMATS_H
#define FILEFORMATS_H

#include <QString>
#include <optional>
#include "Types.h"

/**
 * File format loaders for FM patch files.
 *
 * Supported formats:
 * - TFI (TFM Music Maker) - 42 bytes, direct mapping
 * - DMP (DefleMask) - Variable format, version-dependent
 * - OPN (Generic OPN patch) - Similar to TFI
 */
namespace FileFormats {

    // Detect format from file extension or content
    enum class Format {
        Unknown,
        TFI,
        DMP,
        OPN,
        GEB  // Genesis Engine Bank (our format)
    };

    Format detectFormat(const QString& filePath);

    // Load a single FM patch from file
    std::optional<FMPatch> loadFMPatch(const QString& filePath);

    // Format-specific loaders
    std::optional<FMPatch> loadTFI(const QString& filePath);
    std::optional<FMPatch> loadDMP(const QString& filePath);
    std::optional<FMPatch> loadOPN(const QString& filePath);

    // Save FM patch to file
    bool saveTFI(const QString& filePath, const FMPatch& patch);

    // File filter strings for dialogs
    QString loadFilterString();
    QString saveFilterString();

}  // namespace FileFormats

#endif // FILEFORMATS_H

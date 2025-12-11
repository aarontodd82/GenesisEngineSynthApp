#ifndef PATCHBANK_H
#define PATCHBANK_H

#include <QObject>
#include <array>
#include "Types.h"

/**
 * Manages the 16 FM patch slots and 8 PSG envelope slots.
 * Mirrors the device's RAM storage.
 */
class PatchBank : public QObject
{
    Q_OBJECT

public:
    static constexpr int FM_SLOT_COUNT = 16;
    static constexpr int PSG_SLOT_COUNT = 8;

    explicit PatchBank(QObject* parent = nullptr);

    // FM Patches
    const FMPatch& fmPatch(int slot) const;
    void setFMPatch(int slot, const FMPatch& patch);
    QString fmPatchName(int slot) const;

    // PSG Envelopes
    const PSGEnvelope& psgEnvelope(int slot) const;
    void setPSGEnvelope(int slot, const PSGEnvelope& env);
    QString psgEnvelopeName(int slot) const;

    // Bank file I/O
    bool saveBank(const QString& filePath) const;
    bool loadBank(const QString& filePath);

    // Initialize with default patches
    void loadDefaults();

    // Check if modified since last save
    bool isModified() const { return m_modified; }
    void clearModified() { m_modified = false; }

signals:
    void fmPatchChanged(int slot);
    void psgEnvelopeChanged(int slot);
    void bankLoaded();
    void modifiedChanged(bool modified);

private:
    std::array<FMPatch, FM_SLOT_COUNT> m_fmPatches;
    std::array<PSGEnvelope, PSG_SLOT_COUNT> m_psgEnvelopes;
    bool m_modified = false;

    void setModified(bool modified);
};

#endif // PATCHBANK_H

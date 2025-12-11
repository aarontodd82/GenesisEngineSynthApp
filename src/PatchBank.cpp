#include "PatchBank.h"
#include <QFile>
#include <QDataStream>
#include <QDebug>

// Magic header for bank files
static constexpr char BANK_MAGIC[] = "GEB1";  // Genesis Engine Bank v1

PatchBank::PatchBank(QObject* parent)
    : QObject(parent)
{
    loadDefaults();
}

const FMPatch& PatchBank::fmPatch(int slot) const
{
    static FMPatch empty;
    if (slot < 0 || slot >= FM_SLOT_COUNT) return empty;
    return m_fmPatches[slot];
}

void PatchBank::setFMPatch(int slot, const FMPatch& patch)
{
    if (slot < 0 || slot >= FM_SLOT_COUNT) return;

    m_fmPatches[slot] = patch;
    setModified(true);
    emit fmPatchChanged(slot);
}

QString PatchBank::fmPatchName(int slot) const
{
    if (slot < 0 || slot >= FM_SLOT_COUNT) return QString();

    const FMPatch& patch = m_fmPatches[slot];
    if (!patch.name.isEmpty()) {
        return patch.name;
    }
    return QString("Patch %1").arg(slot);
}

const PSGEnvelope& PatchBank::psgEnvelope(int slot) const
{
    static PSGEnvelope empty;
    if (slot < 0 || slot >= PSG_SLOT_COUNT) return empty;
    return m_psgEnvelopes[slot];
}

void PatchBank::setPSGEnvelope(int slot, const PSGEnvelope& env)
{
    if (slot < 0 || slot >= PSG_SLOT_COUNT) return;

    m_psgEnvelopes[slot] = env;
    setModified(true);
    emit psgEnvelopeChanged(slot);
}

QString PatchBank::psgEnvelopeName(int slot) const
{
    if (slot < 0 || slot >= PSG_SLOT_COUNT) return QString();

    const PSGEnvelope& env = m_psgEnvelopes[slot];
    if (!env.name.isEmpty()) {
        return env.name;
    }
    return QString("Envelope %1").arg(slot);
}

bool PatchBank::saveBank(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }

    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);

    // Write magic header
    out.writeRawData(BANK_MAGIC, 4);

    // Write counts
    out << static_cast<quint8>(FM_SLOT_COUNT);
    out << static_cast<quint8>(PSG_SLOT_COUNT);

    // Write FM patches
    for (int i = 0; i < FM_SLOT_COUNT; i++) {
        const FMPatch& patch = m_fmPatches[i];

        // Write name (length-prefixed string)
        QByteArray nameBytes = patch.name.toUtf8();
        out << static_cast<quint8>(nameBytes.size());
        out.writeRawData(nameBytes.constData(), nameBytes.size());

        // Write patch data (42 bytes)
        auto bytes = patch.toBytes();
        out.writeRawData(reinterpret_cast<const char*>(bytes.data()), 42);
    }

    // Write PSG envelopes
    for (int i = 0; i < PSG_SLOT_COUNT; i++) {
        const PSGEnvelope& env = m_psgEnvelopes[i];

        // Write name
        QByteArray nameBytes = env.name.toUtf8();
        out << static_cast<quint8>(nameBytes.size());
        out.writeRawData(nameBytes.constData(), nameBytes.size());

        // Write envelope data
        out << env.length;
        out << env.loopStart;
        out.writeRawData(reinterpret_cast<const char*>(env.data.data()), 64);
    }

    file.close();
    qDebug() << "Saved bank to" << filePath;
    return true;
}

bool PatchBank::loadBank(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file for reading:" << filePath;
        return false;
    }

    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);

    // Read and verify magic header
    char magic[4];
    in.readRawData(magic, 4);
    if (memcmp(magic, BANK_MAGIC, 4) != 0) {
        qWarning() << "Invalid bank file format";
        return false;
    }

    // Read counts
    quint8 fmCount, psgCount;
    in >> fmCount >> psgCount;

    // Read FM patches
    int fmToRead = qMin(static_cast<int>(fmCount), FM_SLOT_COUNT);
    for (int i = 0; i < fmToRead; i++) {
        // Read name
        quint8 nameLen;
        in >> nameLen;
        QByteArray nameBytes(nameLen, 0);
        in.readRawData(nameBytes.data(), nameLen);
        m_fmPatches[i].name = QString::fromUtf8(nameBytes);

        // Read patch data
        uint8_t patchData[42];
        in.readRawData(reinterpret_cast<char*>(patchData), 42);
        FMPatch patch = FMPatch::fromBytes(patchData);
        patch.name = m_fmPatches[i].name;
        m_fmPatches[i] = patch;
    }

    // Read PSG envelopes
    int psgToRead = qMin(static_cast<int>(psgCount), PSG_SLOT_COUNT);
    for (int i = 0; i < psgToRead; i++) {
        // Read name
        quint8 nameLen;
        in >> nameLen;
        QByteArray nameBytes(nameLen, 0);
        in.readRawData(nameBytes.data(), nameLen);
        m_psgEnvelopes[i].name = QString::fromUtf8(nameBytes);

        // Read envelope data
        in >> m_psgEnvelopes[i].length;
        in >> m_psgEnvelopes[i].loopStart;
        in.readRawData(reinterpret_cast<char*>(m_psgEnvelopes[i].data.data()), 64);
    }

    file.close();
    setModified(false);
    emit bankLoaded();
    qDebug() << "Loaded bank from" << filePath;
    return true;
}

void PatchBank::loadDefaults()
{
    // Default FM Patches (matching DefaultPatches.h from Arduino)

    // Patch 0: Bright EP
    m_fmPatches[0] = FMPatch();
    m_fmPatches[0].name = "Bright EP";
    m_fmPatches[0].algorithm = 5;
    m_fmPatches[0].feedback = 6;
    m_fmPatches[0].op[0] = {1, 3, 35, 1, 31, 12, 0, 6, 2, 0};
    m_fmPatches[0].op[1] = {1, 3, 25, 1, 31, 8, 2, 7, 2, 0};
    m_fmPatches[0].op[2] = {2, 3, 28, 1, 31, 10, 2, 7, 3, 0};
    m_fmPatches[0].op[3] = {1, 3, 20, 1, 31, 10, 2, 8, 2, 0};

    // Patch 1: Synth Bass
    m_fmPatches[1] = FMPatch();
    m_fmPatches[1].name = "Synth Bass";
    m_fmPatches[1].algorithm = 0;
    m_fmPatches[1].feedback = 5;
    m_fmPatches[1].op[0] = {0, 3, 25, 0, 31, 8, 0, 5, 1, 0};
    m_fmPatches[1].op[1] = {1, 3, 30, 0, 31, 10, 0, 5, 2, 0};
    m_fmPatches[1].op[2] = {0, 3, 20, 0, 31, 6, 0, 5, 1, 0};
    m_fmPatches[1].op[3] = {1, 3, 15, 0, 31, 12, 2, 7, 3, 0};

    // Patch 2: Brass
    m_fmPatches[2] = FMPatch();
    m_fmPatches[2].name = "Brass";
    m_fmPatches[2].algorithm = 4;
    m_fmPatches[2].feedback = 4;
    m_fmPatches[2].op[0] = {1, 3, 40, 1, 25, 5, 0, 4, 1, 0};
    m_fmPatches[2].op[1] = {1, 3, 20, 1, 28, 6, 1, 5, 2, 0};
    m_fmPatches[2].op[2] = {2, 4, 35, 1, 25, 5, 0, 4, 1, 0};
    m_fmPatches[2].op[3] = {1, 2, 18, 1, 28, 6, 1, 5, 2, 0};

    // Patch 3: Lead Synth
    m_fmPatches[3] = FMPatch();
    m_fmPatches[3].name = "Lead Synth";
    m_fmPatches[3].algorithm = 7;
    m_fmPatches[3].feedback = 0;
    m_fmPatches[3].op[0] = {1, 3, 28, 2, 31, 8, 0, 6, 2, 0};
    m_fmPatches[3].op[1] = {2, 4, 30, 2, 31, 10, 0, 6, 3, 0};
    m_fmPatches[3].op[2] = {4, 2, 35, 2, 31, 12, 0, 6, 4, 0};
    m_fmPatches[3].op[3] = {1, 3, 25, 2, 31, 8, 0, 6, 2, 0};

    // Patch 4: Organ
    m_fmPatches[4] = FMPatch();
    m_fmPatches[4].name = "Organ";
    m_fmPatches[4].algorithm = 7;
    m_fmPatches[4].feedback = 0;
    m_fmPatches[4].op[0] = {1, 3, 25, 0, 31, 0, 0, 8, 0, 0};
    m_fmPatches[4].op[1] = {2, 3, 30, 0, 31, 0, 0, 8, 0, 0};
    m_fmPatches[4].op[2] = {4, 3, 35, 0, 31, 0, 0, 8, 0, 0};
    m_fmPatches[4].op[3] = {8, 3, 40, 0, 31, 0, 0, 8, 0, 0};

    // Patch 5: Strings
    m_fmPatches[5] = FMPatch();
    m_fmPatches[5].name = "Strings";
    m_fmPatches[5].algorithm = 2;
    m_fmPatches[5].feedback = 3;
    m_fmPatches[5].op[0] = {1, 3, 35, 0, 18, 4, 0, 4, 1, 0};
    m_fmPatches[5].op[1] = {2, 4, 40, 0, 20, 5, 0, 4, 2, 0};
    m_fmPatches[5].op[2] = {3, 2, 45, 0, 22, 6, 0, 4, 2, 0};
    m_fmPatches[5].op[3] = {1, 3, 22, 0, 16, 6, 1, 5, 2, 0};

    // Patch 6: Pluck
    m_fmPatches[6] = FMPatch();
    m_fmPatches[6].name = "Pluck";
    m_fmPatches[6].algorithm = 0;
    m_fmPatches[6].feedback = 6;
    m_fmPatches[6].op[0] = {1, 3, 28, 2, 31, 15, 5, 8, 5, 0};
    m_fmPatches[6].op[1] = {3, 3, 35, 2, 31, 18, 6, 8, 6, 0};
    m_fmPatches[6].op[2] = {1, 4, 30, 2, 31, 16, 5, 8, 5, 0};
    m_fmPatches[6].op[3] = {1, 3, 18, 2, 31, 14, 4, 9, 4, 0};

    // Patch 7: Bell
    m_fmPatches[7] = FMPatch();
    m_fmPatches[7].name = "Bell";
    m_fmPatches[7].algorithm = 4;
    m_fmPatches[7].feedback = 3;
    m_fmPatches[7].op[0] = {1, 3, 30, 2, 31, 6, 2, 5, 3, 0};
    m_fmPatches[7].op[1] = {1, 3, 22, 2, 31, 8, 2, 6, 3, 0};
    m_fmPatches[7].op[2] = {7, 6, 45, 2, 31, 10, 3, 6, 5, 0};
    m_fmPatches[7].op[3] = {3, 0, 25, 2, 31, 9, 2, 7, 4, 0};

    // Initialize remaining FM slots as empty
    for (int i = 8; i < FM_SLOT_COUNT; i++) {
        m_fmPatches[i] = FMPatch();
        m_fmPatches[i].name = QString("Empty %1").arg(i);
    }

    // Default PSG Envelopes

    // Envelope 0: Short pluck
    m_psgEnvelopes[0].name = "Pluck";
    m_psgEnvelopes[0].length = 10;
    m_psgEnvelopes[0].loopStart = 0xFF;
    m_psgEnvelopes[0].data = {0x00, 0x01, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x0F};

    // Envelope 1: Sustain
    m_psgEnvelopes[1].name = "Sustain";
    m_psgEnvelopes[1].length = 4;
    m_psgEnvelopes[1].loopStart = 0;
    m_psgEnvelopes[1].data = {0x00, 0x00, 0x00, 0x00};

    // Envelope 2: Slow attack
    m_psgEnvelopes[2].name = "Slow Attack";
    m_psgEnvelopes[2].length = 12;
    m_psgEnvelopes[2].loopStart = 8;
    m_psgEnvelopes[2].data = {0x0F, 0x0C, 0x0A, 0x08, 0x06, 0x04, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00};

    // Envelope 3: Tremolo
    m_psgEnvelopes[3].name = "Tremolo";
    m_psgEnvelopes[3].length = 8;
    m_psgEnvelopes[3].loopStart = 0;
    m_psgEnvelopes[3].data = {0x00, 0x02, 0x04, 0x02, 0x00, 0x02, 0x04, 0x02};

    // Initialize remaining PSG slots as empty
    for (int i = 4; i < PSG_SLOT_COUNT; i++) {
        m_psgEnvelopes[i] = PSGEnvelope();
        m_psgEnvelopes[i].name = QString("Empty %1").arg(i);
        m_psgEnvelopes[i].length = 1;
        m_psgEnvelopes[i].data[0] = 0x0F;  // Silent
    }

    setModified(false);
}

void PatchBank::setModified(bool modified)
{
    if (m_modified != modified) {
        m_modified = modified;
        emit modifiedChanged(modified);
    }
}

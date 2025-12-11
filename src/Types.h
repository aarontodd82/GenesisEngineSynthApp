#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <array>
#include <QString>

/**
 * FM Operator parameters (matches Arduino FMPatch.h)
 * 10 bytes per operator, TFI-compatible order
 */
struct FMOperator {
    uint8_t mul = 1;    // Multiplier (0-15, 0=0.5x)
    uint8_t dt = 3;     // Detune (0-7, 3=center)
    uint8_t tl = 32;    // Total Level (0-127, 0=loudest)
    uint8_t rs = 0;     // Rate Scaling (0-3)
    uint8_t ar = 31;    // Attack Rate (0-31)
    uint8_t dr = 8;     // Decay Rate (0-31)
    uint8_t sr = 0;     // Sustain Rate (0-31)
    uint8_t rr = 6;     // Release Rate (0-15)
    uint8_t sl = 2;     // Sustain Level (0-15)
    uint8_t ssg = 0;    // SSG-EG mode (0-15, 0=off)

    bool operator==(const FMOperator& other) const {
        return mul == other.mul && dt == other.dt && tl == other.tl &&
               rs == other.rs && ar == other.ar && dr == other.dr &&
               sr == other.sr && rr == other.rr && sl == other.sl &&
               ssg == other.ssg;
    }
};

/**
 * FM Patch (42 bytes total, TFI-compatible)
 * Matches Arduino FMPatch struct exactly
 */
struct FMPatch {
    uint8_t algorithm = 0;  // Algorithm (0-7)
    uint8_t feedback = 0;   // Feedback (0-7)
    std::array<FMOperator, 4> op;  // Operators in TFI order: S1, S3, S2, S4
    QString name;           // Patch name (not sent to device, local only)

    bool operator==(const FMPatch& other) const {
        return algorithm == other.algorithm && feedback == other.feedback &&
               op == other.op;
    }

    // Serialize to 42-byte TFI format for SysEx
    std::array<uint8_t, 42> toBytes() const {
        std::array<uint8_t, 42> data;
        data[0] = algorithm;
        data[1] = feedback;
        for (int i = 0; i < 4; i++) {
            int offset = 2 + i * 10;
            data[offset + 0] = op[i].mul;
            data[offset + 1] = op[i].dt;
            data[offset + 2] = op[i].tl;
            data[offset + 3] = op[i].rs;
            data[offset + 4] = op[i].ar;
            data[offset + 5] = op[i].dr;
            data[offset + 6] = op[i].sr;
            data[offset + 7] = op[i].rr;
            data[offset + 8] = op[i].sl;
            data[offset + 9] = op[i].ssg;
        }
        return data;
    }

    // Deserialize from 42-byte TFI format
    static FMPatch fromBytes(const uint8_t* data) {
        FMPatch patch;
        patch.algorithm = data[0];
        patch.feedback = data[1];
        for (int i = 0; i < 4; i++) {
            int offset = 2 + i * 10;
            patch.op[i].mul = data[offset + 0];
            patch.op[i].dt = data[offset + 1];
            patch.op[i].tl = data[offset + 2];
            patch.op[i].rs = data[offset + 3];
            patch.op[i].ar = data[offset + 4];
            patch.op[i].dr = data[offset + 5];
            patch.op[i].sr = data[offset + 6];
            patch.op[i].rr = data[offset + 7];
            patch.op[i].sl = data[offset + 8];
            patch.op[i].ssg = data[offset + 9];
        }
        return patch;
    }
};

/**
 * PSG Software Envelope (matches Arduino PSGEnvelope.h)
 */
struct PSGEnvelope {
    std::array<uint8_t, 64> data = {0};  // Envelope data (max 64 steps)
    uint8_t length = 1;                   // Actual length (1-64)
    uint8_t loopStart = 0xFF;             // Loop start position (0xFF = no loop)
    QString name;                         // Envelope name (local only)

    bool operator==(const PSGEnvelope& other) const {
        return data == other.data && length == other.length &&
               loopStart == other.loopStart;
    }
};

/**
 * SysEx command definitions (must match Arduino firmware)
 */
namespace SysEx {
    constexpr uint8_t MANUFACTURER_ID = 0x7D;  // Educational/development
    constexpr uint8_t DEVICE_ID = 0x00;

    // Commands (Host → Device)
    constexpr uint8_t CMD_LOAD_FM_PATCH = 0x01;      // Load FM patch to channel
    constexpr uint8_t CMD_LOAD_PSG_ENV = 0x02;       // Load PSG envelope
    constexpr uint8_t CMD_STORE_FM_PATCH = 0x03;    // Store FM patch to slot
    constexpr uint8_t CMD_RECALL_PATCH = 0x04;      // Recall patch to channel
    constexpr uint8_t CMD_REQUEST_PATCH = 0x10;     // Request patch dump
    constexpr uint8_t CMD_REQUEST_ALL = 0x11;       // Request all patches
    constexpr uint8_t CMD_SET_MODE = 0x12;          // Set synth mode
    constexpr uint8_t CMD_PING = 0x13;              // Ping/identify

    // Responses (Device → Host)
    constexpr uint8_t RESP_PATCH_DUMP = 0x80;       // Patch dump response
    constexpr uint8_t RESP_IDENTITY = 0x81;         // Identity response
}

/**
 * Synth modes
 */
enum class SynthMode {
    Multi = 0,  // 6 independent FM channels
    Poly = 1    // 6-voice polyphonic on channel 1
};

/**
 * Connection state
 */
enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Error
};

#endif // TYPES_H

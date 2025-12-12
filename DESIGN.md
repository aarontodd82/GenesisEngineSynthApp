# GenesisEngine Companion App - Implementation Design

## Overview

Cross-platform companion application for the GenesisEngine MIDISynth that provides:
1. **Virtual MIDI port** ("Genesis Engine") for AVR boards lacking native USB MIDI
2. **Serial-to-MIDI bridge** forwarding MIDI from DAWs to Arduino
3. **FM Patch Editor** with visual operator editing
4. **Patch Management** - load TFI/DMP/OPN files, manage 16 slots
5. **PSG Envelope Editor** with visual envelope curves
6. **Universal Control** - works with both AVR (required) and Teensy (optional enhancement)

## Technology Stack

- **Framework**: C++ with Qt 6
- **Virtual MIDI**:
  - Windows: loopMIDI API or Windows MIDI Services
  - macOS: CoreMIDI virtual ports
  - Linux: ALSA sequencer virtual ports
- **Serial**: Qt Serial Port module
- **Build**: CMake with Qt deployment tools
- **Distribution**: Static-linked single executable per platform

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    GenesisEngine Companion                       │
├─────────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────────────┐│
│  │ Virtual MIDI │  │ Serial Port  │  │     Main Window        ││
│  │    Driver    │  │   Manager    │  │  ┌──────────────────┐  ││
│  │              │  │              │  │  │ FM Patch Editor  │  ││
│  │ - Windows    │  │ - Auto-detect│  │  │ - Algorithm view │  ││
│  │ - macOS      │  │ - Baud 115200│  │  │ - Operator knobs │  ││
│  │ - Linux      │  │ - Reconnect  │  │  │ - Envelope curves│  ││
│  └──────┬───────┘  └──────┬───────┘  │  └──────────────────┘  ││
│         │                 │          │  ┌──────────────────┐  ││
│         └────────┬────────┘          │  │ PSG Env Editor   │  ││
│                  │                   │  └──────────────────┘  ││
│         ┌────────▼────────┐          │  ┌──────────────────┐  ││
│         │  MIDI Router    │          │  │ Patch Bank       │  ││
│         │ - Note on/off   │◄────────►│  │ - 16 FM slots    │  ││
│         │ - CC            │          │  │ - 8 PSG slots    │  ││
│         │ - Program chg   │          │  │ - File I/O       │  ││
│         │ - SysEx         │          │  └──────────────────┘  ││
│         └────────┬────────┘          └────────────────────────┘│
│                  │                                              │
└──────────────────┼──────────────────────────────────────────────┘
                   │ Serial (115200 baud)
                   ▼
            ┌──────────────┐
            │   Arduino    │
            │  (AVR/Teensy)│
            └──────────────┘
```

## Serial Protocol

### For AVR (No USB MIDI)

The companion app will send raw MIDI bytes over serial. The AVR firmware will need a small addition to parse serial MIDI alongside (or instead of) USB MIDI.

**Serial MIDI Format** (standard MIDI bytes):
```
Note On:      0x9n kk vv    (n=channel 0-F, kk=note, vv=velocity)
Note Off:     0x8n kk vv
Control:      0xBn cc vv    (cc=controller, vv=value)
Program:      0xCn pp       (pp=program number)
Pitch Bend:   0xEn ll hh    (ll=LSB, hh=MSB)
SysEx:        0xF0 ... 0xF7
```

### SysEx Commands (Existing + New)

Current SysEx format: `F0 7D 00 <cmd> <data...> F7`

| Cmd | Name | Data | Description |
|-----|------|------|-------------|
| 0x01 | Load FM patch to channel | `<ch> <42 bytes>` | Immediate load |
| 0x02 | Load PSG envelope | `<ch> <len> <loop> <data...>` | Load envelope |
| 0x03 | Store FM patch to slot | `<slot> <42 bytes>` | Store to RAM |
| 0x04 | Recall patch to channel | `<ch> <slot>` | Recall from slot |
| 0x10 | **Request patch dump** | `<slot>` | **NEW**: Device replies with patch |
| 0x11 | **Request all patches** | - | **NEW**: Device dumps all 16 slots |
| 0x12 | **Set synth mode** | `<mode>` | **NEW**: 0=Multi, 1=Poly |
| 0x13 | **Ping/identify** | - | **NEW**: Device replies with ID |

### Response Messages (Device → Host)

```
F0 7D 00 80 <slot> <42 bytes> F7    - Patch dump response
F0 7D 00 81 <mode> <version> F7    - Identity response
```

## Firmware Modifications Required

### For AVR Support

Add serial MIDI parsing to MIDISynth.ino:

```cpp
// In loop(), add serial MIDI reading
void loop() {
    // Existing USB MIDI (Teensy only)
    #ifdef USB_MIDI
    while (usbMIDI.read()) { ... }
    #endif

    // Serial MIDI (all platforms)
    processSerialMIDI();

    // PSG envelopes
    updatePSGEnvelopes();
}

// New serial MIDI parser
void processSerialMIDI() {
    static uint8_t buffer[256];
    static uint8_t bufPos = 0;
    static uint8_t expectedLen = 0;

    while (Serial.available()) {
        uint8_t b = Serial.read();
        // ... parse MIDI stream ...
    }
}
```

### Conditional Compilation

```cpp
// Platform detection
#if defined(TEENSYDUINO) && defined(USB_MIDI)
    #define HAS_USB_MIDI 1
#else
    #define HAS_USB_MIDI 0
#endif
```

## GUI Components

### 1. Main Window Layout

```
┌────────────────────────────────────────────────────────────────┐
│ File  Edit  MIDI  Help                          [Connected: COM3]│
├────────────────────────────────────────────────────────────────┤
│ ┌─────────────────────────────────────────────┐ ┌────────────┐ │
│ │           FM Patch Editor                    │ │ Patch Bank │ │
│ │  ┌─────────────────────────────────────────┐│ │            │ │
│ │  │         Algorithm Display               ││ │ 0: Bright EP│ │
│ │  │    [1]──[2]                             ││ │ 1: Syn Bass │ │
│ │  │      \  /                               ││ │ 2: Brass    │ │
│ │  │      [3]──[4]──►OUT                     ││ │ 3: Lead     │ │
│ │  └─────────────────────────────────────────┘│ │ 4: Organ   │ │
│ │  Algorithm: [▼ 4] Feedback: [▼ 5]          │ │ 5: Strings  │ │
│ │                                             │ │ 6: Pluck    │ │
│ │  ┌─OP1 ──┐┌─OP2 ──┐┌─OP3 ──┐┌─OP4 ──┐     │ │ 7: Bell     │ │
│ │  │MUL [1]││MUL [2]││MUL [3]││MUL [1]│     │ │ 8: ----     │ │
│ │  │DT  [3]││DT  [4]││DT  [2]││DT  [3]│     │ │ ...         │ │
│ │  │TL [35]││TL [28]││TL [45]││TL [22]│     │ │             │ │
│ │  │RS  [1]││RS  [1]││RS  [0]││RS  [0]│     │ │ [Load TFI]  │ │
│ │  │AR [31]││AR [31]││AR [22]││AR [16]│     │ │ [Save TFI]  │ │
│ │  │DR [12]││DR [10]││DR  [6]││DR  [6]│     │ │ [Send to HW]│ │
│ │  │SR  [0]││SR  [2]││SR  [0]││SR  [1]│     │ │             │ │
│ │  │RR  [6]││RR  [7]││RR  [4]││RR  [5]│     │ │ ──────────  │ │
│ │  │SL  [2]││SL  [3]││SL  [2]││SL  [2]│     │ │ Target:     │ │
│ │  │SSG [0]││SSG [0]││SSG [0]││SSG [0]│     │ │ [▼ Channel 1]│ │
│ │  └───────┘└───────┘└───────┘└───────┘     │ │ [▼ Slot 0]  │ │
│ └─────────────────────────────────────────────┘ └────────────┘ │
│ ┌─────────────────────────────────────────────┐ ┌────────────┐ │
│ │         ADSR Envelope Visualization          │ │   Status   │ │
│ │    ╱╲                                        │ │ MIDI: ●    │ │
│ │   ╱  ╲_______________                        │ │ Serial: ●  │ │
│ │  ╱                   ╲                       │ │ Mode: Multi│ │
│ └─────────────────────────────────────────────┘ └────────────┘ │
├────────────────────────────────────────────────────────────────┤
│ [PSG Envelopes]  [MIDI Monitor]  [Settings]                     │
└────────────────────────────────────────────────────────────────┘
```

### 2. FM Patch Editor Features

- **Algorithm selector** (0-7) with visual routing diagram
- **Feedback knob** (0-7)
- **4 Operator panels** with:
  - MUL (0-15), DT (-3 to +3 displayed, 0-7 stored)
  - TL (0-127), RS (0-3)
  - AR (0-31), DR (0-31), SR (0-31), RR (0-15)
  - SL (0-15), SSG-EG (0-15)
- **ADSR envelope preview** (calculated from AR/DR/SR/RR/SL)
- **Carrier highlighting** based on algorithm

### 3. PSG Envelope Editor

- **64-step volume envelope** editor (click/drag)
- **Loop point** marker
- **Preview playback** at 60Hz
- **Preset envelopes** (pluck, sustain, pad, tremolo)

### 4. Settings Panel

- **Serial port** selection (auto-detect Arduino)
- **Virtual MIDI** enable/disable
- **MIDI input** selection (for forwarding)
- **Synth mode** toggle (Multi/Poly)
- **Theme** (light/dark)

## File Format Support

### TFI (TFM Music Maker)
- 42 bytes: alg, fb, then 4×10 operator bytes
- Direct mapping to FMPatch struct

### DMP (DefleMask)
- Variable format, version-dependent
- Parse header, extract FM parameters
- Convert operator order if needed

### OPN (Generic OPN patch)
- Similar to TFI with slight variations
- May include additional metadata

### GEB (Genesis Engine Bank) - Custom
```
Header:
  4 bytes: "GEB1" magic
  1 byte:  FM patch count (0-16)
  1 byte:  PSG envelope count (0-8)

FM Patches:
  For each: 42 bytes (TFI format)

PSG Envelopes:
  For each: 66 bytes (length, loopStart, 64 data bytes)
```

## Virtual MIDI Implementation

### Windows (Requires loopMIDI)

**Important**: Windows has no user-space API for creating virtual MIDI ports.
Unlike macOS and Linux, we cannot create virtual ports programmatically.

**Strategy for v1.0**: Require user to install [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html) (free)
- App detects available MIDI ports via RtMidi
- User creates "Genesis Engine" port in loopMIDI
- App connects to that port and forwards to serial

```cpp
// Windows: Connect to existing loopMIDI port (cannot create virtual ports)
#include <rtmidi/RtMidi.h>

class WindowsMIDIInput {
    RtMidiIn* midiIn;
public:
    bool connectToPort(const std::string& portName) {
        midiIn = new RtMidiIn();
        // Find port by name
        for (unsigned int i = 0; i < midiIn->getPortCount(); i++) {
            if (midiIn->getPortName(i).find(portName) != std::string::npos) {
                midiIn->openPort(i);
                midiIn->setCallback(&midiCallback, this);
                return true;
            }
        }
        return false;  // Port not found - prompt user to create in loopMIDI
    }
};
```

**Future options** (see roadmap):
- Bundle virtualMIDI SDK (commercial license ~€50)
- Target Windows MIDI Services (Windows 10 20H2+)

### macOS
```cpp
// CoreMIDI virtual ports (native support)
#include <CoreMIDI/CoreMIDI.h>

MIDIClientRef client;
MIDIEndpointRef virtualSource;

MIDIClientCreate(CFSTR("GenesisEngine"), nullptr, nullptr, &client);
MIDISourceCreate(client, CFSTR("Genesis Engine"), &virtualSource);
```

### Linux
```cpp
// ALSA sequencer
#include <alsa/asoundlib.h>

snd_seq_t* seq;
snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0);
snd_seq_set_client_name(seq, "Genesis Engine");
int port = snd_seq_create_simple_port(seq, "Input",
    SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
    SND_SEQ_PORT_TYPE_MIDI_GENERIC);
```

## Build System

### CMakeLists.txt Structure
```cmake
cmake_minimum_required(VERSION 3.16)
project(GenesisEngineCompanion VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core Widgets SerialPort)

# Platform-specific MIDI
if(WIN32)
    find_package(rtmidi REQUIRED)
    target_link_libraries(${PROJECT_NAME} rtmidi)
elseif(APPLE)
    find_library(COREMIDI_LIBRARY CoreMIDI)
    target_link_libraries(${PROJECT_NAME} ${COREMIDI_LIBRARY})
elseif(UNIX)
    find_package(ALSA REQUIRED)
    target_link_libraries(${PROJECT_NAME} ALSA::ALSA)
endif()

add_executable(GenesisEngineCompanion
    src/main.cpp
    src/MainWindow.cpp
    src/FMPatchEditor.cpp
    src/PSGEnvelopeEditor.cpp
    src/PatchBank.cpp
    src/SerialManager.cpp
    src/MIDIRouter.cpp
    src/VirtualMIDI.cpp
    src/FileFormats.cpp
    resources/resources.qrc
)

target_link_libraries(GenesisEngineCompanion
    Qt6::Core Qt6::Widgets Qt6::SerialPort)
```

## Source File Structure

```
companion/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── MainWindow.h/cpp
│   ├── FMPatchEditor.h/cpp      # Operator knobs, algorithm display
│   ├── PSGEnvelopeEditor.h/cpp  # Volume envelope editor
│   ├── PatchBank.h/cpp          # 16 FM + 8 PSG slot management
│   ├── SerialManager.h/cpp      # Serial port handling
│   ├── MIDIRouter.h/cpp         # MIDI message routing
│   ├── VirtualMIDI.h/cpp        # Platform-specific virtual ports
│   ├── FileFormats.h/cpp        # TFI/DMP/OPN/GEB parsing
│   ├── AlgorithmWidget.h/cpp    # Visual algorithm display
│   ├── KnobWidget.h/cpp         # Custom rotary knob
│   └── EnvelopeWidget.h/cpp     # ADSR curve display
├── resources/
│   ├── resources.qrc
│   ├── icons/
│   └── patches/                 # Default patch bank
└── platform/
    ├── windows/                 # Windows-specific (installer, etc.)
    ├── macos/                   # macOS bundle config
    └── linux/                   # AppImage/Flatpak config
```

## Implementation Phases

### Phase 1: Core Infrastructure ✅ COMPLETE
- [x] Qt project setup with CMake
- [x] Serial port manager with auto-detect
- [x] Basic main window layout
- [x] FMPatch and PSGEnvelope data structures

### Phase 2: Patch Editor ✅ COMPLETE
- [x] Operator parameter controls (knobs/spinboxes)
- [x] Algorithm selector with visual display
- [x] ADSR envelope visualization (interactive, draggable)
- [x] Carrier operator highlighting

### Phase 3: Patch Management ✅ COMPLETE
- [x] 16-slot FM patch bank
- [x] 8-slot PSG envelope bank
- [x] TFI file loading/saving
- [x] DMP file loading
- [x] OPN file loading
- [x] GEB bank file format

### Phase 4: MIDI Integration ✅ COMPLETE
- [x] Virtual MIDI port (Windows - requires loopMIDI)
- [x] Virtual MIDI port (macOS - CoreMIDI native)
- [x] Virtual MIDI port (Linux - ALSA native)
- [x] MIDI input selection
- [x] MIDI → Serial forwarding
- [x] SysEx transmission

### Phase 5: Firmware Updates ✅ COMPLETE
- [x] Serial MIDI parser for AVR
- [x] Conditional USB MIDI compilation
- [x] New SysEx commands (dump, identify)
- [x] Unified MIDI handling function

### Phase 6: Polish ✅ COMPLETE
- [x] MIDI activity monitor (RX/TX indicators)
- [x] Connection status indicators
- [x] Dark theme (default)
- [x] Keyboard shortcuts (File menu)
- [x] Remember window state (QSettings)
- [x] Real-time patch preview (Live Edit mode)

### Phase 7: Distribution
- [ ] Windows installer (NSIS or WiX)
- [ ] macOS .app bundle + DMG
- [ ] Linux AppImage
- [x] Documentation (README, DESIGN.md)

## Dependencies

| Library | Purpose | License |
|---------|---------|---------|
| Qt 6 | GUI framework | LGPL |
| RtMidi | Cross-platform MIDI (Windows) | MIT |
| CoreMIDI | macOS MIDI (system) | - |
| ALSA | Linux MIDI | LGPL |

## Testing Strategy

1. **Unit tests**: File format parsing, MIDI message encoding
2. **Integration tests**: Serial communication, SysEx round-trip
3. **Manual testing matrix**:
   - Windows 10/11 + Arduino Uno
   - Windows 10/11 + Teensy 4.0
   - macOS 12+ + Arduino Uno
   - macOS 12+ + Teensy 4.0
   - Ubuntu 22.04 + Arduino Uno
   - Ubuntu 22.04 + Teensy 4.0

## Future Roadmap

Features planned for later phases (post-v1.0):

### Phase 8: Patch Persistence
- [ ] Save device patch state to local file on disconnect
- [ ] Auto-restore patches when device reconnects
- [ ] "Sync" button to pull current patches from device
- [ ] Backup/restore entire device state

### Phase 9: MIDI Learn
- [ ] Right-click any knob → "MIDI Learn"
- [ ] Map external MIDI controllers to patch parameters
- [ ] Save/load MIDI mappings
- [ ] CC pass-through vs. capture mode

### Phase 10: DAC/Sample Support
- [ ] FM Channel 6 DAC mode toggle
- [ ] Load 8-bit PCM samples (WAV import)
- [ ] Sample playback via note trigger
- [ ] Basic sample editor (trim, normalize)
- [ ] Sample bank management

### Phase 11: Windows Native Virtual MIDI
- [ ] Evaluate virtualMIDI SDK licensing
- [ ] Or: Target Windows MIDI Services (Win 10 20H2+)
- [ ] Seamless virtual port creation without loopMIDI

### Phase 12: Advanced Features
- [ ] Patch randomizer with constraints
- [ ] A/B patch comparison
- [ ] Undo/redo for patch edits
- [ ] Patch morph between two patches
- [ ] Export patches as C headers (for PROGMEM)

---

*Document created for GenesisEngine MIDISynth Companion App*
*Target: Cross-platform C++/Qt6 application*

# Genesis Engine Synth App

Companion application for the [GenesisEngine](https://github.com/aarontodd82/GenesisEngine) MIDISynth.

## Features

- **Virtual MIDI Port** - Creates "Genesis Engine" MIDI port for DAWs (macOS/Linux native, Windows requires loopMIDI)
- **MIDI-to-Serial Bridge** - Forwards MIDI from DAWs to Arduino/Teensy over serial
- **FM Patch Editor** - Visual editor for YM2612 FM patches with algorithm display and interactive envelope widgets
- **PSG Envelope Editor** - Volume envelope editor for SN76489 channels
- **Patch Management** - 16 FM slots, 8 PSG slots, bank save/load
- **File Format Support** - Load TFI, DMP, OPN patch files
- **Live Edit Mode** - Real-time patch preview: changes are sent to hardware as you edit
- **MIDI Activity LEDs** - Visual RX/TX indicators show MIDI traffic
- **On-Screen Keyboard** - Test patches directly without external MIDI controller
- **Channel Controls** - Pan (L/C/R) and LFO enable per channel
- **Patch Randomizer** - Generate random FM patches with sensible constraints
- **Panic Button** - All Notes Off to stop stuck notes
- **Smart Board Detection** - Automatically detects Teensy vs Arduino and adjusts MIDI routing

## Requirements

- Qt 6.x
- CMake 3.16+
- Platform-specific:
  - **macOS**: CoreMIDI (included with OS)
  - **Linux**: ALSA development libraries (`libasound2-dev`)
  - **Windows**: [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html) for virtual ports

## Building

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Windows

```bash
cmake -DCMAKE_PREFIX_PATH="C:/Qt/6.x/msvc2022_64" ..
cmake --build . --config Release
```

### macOS

```bash
cmake -DCMAKE_PREFIX_PATH="/usr/local/opt/qt6" ..
cmake --build .
```

### Linux

```bash
sudo apt install qt6-base-dev qt6-serialport-dev libasound2-dev
cmake ..
cmake --build .
```

## Usage

1. Connect your GenesisEngine hardware via USB
2. Launch the app and select the serial port
3. Click "Connect"
4. (Optional) Create a virtual MIDI port or select an existing MIDI input
5. Enable "Forward to device" to send MIDI to the hardware
6. Edit patches in the FM Patch Editor tab
7. Click "Send to Device" to upload patches

### Live Edit Mode

Enable "Live Edit (auto-send on change)" to hear patch changes in real-time:
- Every parameter tweak is instantly sent to the hardware
- Use the on-screen keyboard or external MIDI to trigger notes while editing
- Useful for sound design and quick iteration

### MIDI Activity LEDs

The RX/TX indicators in the MIDI section show MIDI traffic:
- **RX** (green): Flashes when MIDI is received from the selected input
- **TX** (yellow): Flashes when MIDI is sent to the hardware

### Teensy vs Arduino Detection

When you connect, the app detects your board type:
- **Teensy**: MIDI forwarding is disabled by default (your DAW connects directly to Teensy MIDI). Use the app for patch editing.
- **Arduino**: MIDI forwarding is enabled by default. Create a virtual MIDI port for your DAW to connect to.

### Panic Button

The red "PANIC" button below the keyboard sends All Notes Off and All Sound Off to all channels - useful when notes get stuck.

### Randomize Patch

Click "Randomize Patch" to generate a random FM patch. The randomizer uses sensible constraints (e.g., reasonable envelope times, proper carrier levels) to produce usable sounds.

## Hardware Compatibility

- **Teensy 4.x**: Works standalone with USB MIDI, companion app optional
- **Arduino Uno/Mega**: Requires companion app for MIDI input (no USB MIDI support)

## Protocol

Communication uses standard MIDI over serial at 115200 baud. SysEx commands use manufacturer ID `0x7D` (educational/development).

See [DESIGN.md](DESIGN.md) for protocol details.

## License

MIT License

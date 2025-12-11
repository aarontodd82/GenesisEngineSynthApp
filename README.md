# Genesis Engine Synth App

Companion application for the [GenesisEngine](https://github.com/yourusername/GenesisEngine) MIDISynth.

## Features

- **Virtual MIDI Port** - Creates "Genesis Engine" MIDI port for DAWs (macOS/Linux native, Windows requires loopMIDI)
- **MIDI-to-Serial Bridge** - Forwards MIDI from DAWs to Arduino/Teensy over serial
- **FM Patch Editor** - Visual editor for YM2612 FM patches with algorithm display
- **PSG Envelope Editor** - Volume envelope editor for SN76489 channels
- **Patch Management** - 16 FM slots, 8 PSG slots, bank save/load
- **File Format Support** - Load TFI, DMP, OPN patch files

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

## Hardware Compatibility

- **Teensy 4.x**: Works standalone with USB MIDI, companion app optional
- **Arduino Uno/Mega**: Requires companion app for MIDI input (no USB MIDI support)

## Protocol

Communication uses standard MIDI over serial at 115200 baud. SysEx commands use manufacturer ID `0x7D` (educational/development).

See [DESIGN.md](DESIGN.md) for protocol details.

## License

MIT License

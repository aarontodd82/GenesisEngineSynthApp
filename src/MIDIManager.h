#ifndef MIDIMANAGER_H
#define MIDIMANAGER_H

#include <QObject>
#include <QStringList>
#include <vector>
#include <memory>
#include "Types.h"

// Forward declarations for platform-specific implementation
class MIDIManagerPrivate;

/**
 * Cross-platform MIDI input manager.
 *
 * Platform support:
 * - macOS: CoreMIDI with native virtual port creation
 * - Linux: ALSA with native virtual port creation
 * - Windows: RtMidi or WinMM (requires loopMIDI for virtual ports)
 */
class MIDIManager : public QObject
{
    Q_OBJECT

public:
    explicit MIDIManager(QObject* parent = nullptr);
    ~MIDIManager();

    // Port enumeration
    QStringList availableInputPorts() const;
    void refreshPorts();

    // Port connection
    bool openInputPort(int portIndex);
    bool openInputPort(const QString& portName);
    void closeInputPort();
    bool isInputOpen() const;
    QString currentInputPort() const;

    // Virtual port creation (macOS/Linux only)
    bool canCreateVirtualPorts() const;
    bool createVirtualInputPort(const QString& name = "Genesis Engine");
    void destroyVirtualInputPort();
    bool hasVirtualPort() const;

    // Enable/disable MIDI forwarding
    void setForwardingEnabled(bool enabled);
    bool isForwardingEnabled() const;

signals:
    // Raw MIDI data received (for forwarding to serial)
    void midiReceived(const std::vector<uint8_t>& message);

    // Parsed MIDI events (for UI display)
    void noteOnReceived(uint8_t channel, uint8_t note, uint8_t velocity);
    void noteOffReceived(uint8_t channel, uint8_t note, uint8_t velocity);
    void controlChangeReceived(uint8_t channel, uint8_t cc, uint8_t value);
    void programChangeReceived(uint8_t channel, uint8_t program);
    void pitchBendReceived(uint8_t channel, uint16_t value);
    void sysExReceived(const std::vector<uint8_t>& data);

    // Status
    void portsChanged();
    void inputOpened(const QString& portName);
    void inputClosed();
    void error(const QString& message);

private:
    std::unique_ptr<MIDIManagerPrivate> d;
    bool m_forwardingEnabled;
};

#endif // MIDIMANAGER_H

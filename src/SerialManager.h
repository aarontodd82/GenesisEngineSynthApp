#ifndef SERIALMANAGER_H
#define SERIALMANAGER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QByteArray>
#include <vector>
#include "Types.h"

/**
 * Manages serial communication with the GenesisEngine device.
 * Handles MIDI message transmission and SysEx commands.
 */
class SerialManager : public QObject
{
    Q_OBJECT

public:
    explicit SerialManager(QObject* parent = nullptr);
    ~SerialManager();

    // Connection management
    QStringList availablePorts() const;
    bool connect(const QString& portName);
    void disconnect();
    bool isConnected() const;
    QString connectedPort() const;

    // Raw MIDI message sending
    void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
    void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity = 0);
    void sendControlChange(uint8_t channel, uint8_t cc, uint8_t value);
    void sendProgramChange(uint8_t channel, uint8_t program);
    void sendPitchBend(uint8_t channel, uint16_t value);
    void sendRawMIDI(const std::vector<uint8_t>& data);

    // SysEx commands
    void sendFMPatchToChannel(uint8_t channel, const FMPatch& patch);
    void sendFMPatchToSlot(uint8_t slot, const FMPatch& patch);
    void sendPSGEnvelope(uint8_t channel, const PSGEnvelope& env);
    void recallPatchToChannel(uint8_t channel, uint8_t slot);
    void requestPatchDump(uint8_t slot);
    void requestAllPatches();
    void setSynthMode(SynthMode mode);
    void ping();

signals:
    void connected();
    void disconnected();
    void connectionError(const QString& error);
    void connectionStateChanged(ConnectionState state);

    // Data received from device
    void patchReceived(uint8_t slot, const FMPatch& patch);
    void identityReceived(uint8_t mode, uint8_t version);
    void midiDataReceived(const QByteArray& data);

private slots:
    void onReadyRead();
    void onError(QSerialPort::SerialPortError error);
    void onAutoDetectTimer();

private:
    void sendSysEx(const std::vector<uint8_t>& data);
    void processSysEx(const QByteArray& sysex);
    bool isArduinoPort(const QSerialPortInfo& info) const;

    QSerialPort* m_port;
    QTimer* m_autoDetectTimer;
    QByteArray m_rxBuffer;
    bool m_inSysEx;
    ConnectionState m_state;

    static constexpr int BAUD_RATE = 115200;
    static constexpr int AUTO_DETECT_INTERVAL_MS = 2000;
};

#endif // SERIALMANAGER_H

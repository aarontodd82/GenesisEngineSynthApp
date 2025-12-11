#include "SerialManager.h"
#include <QDebug>

SerialManager::SerialManager(QObject* parent)
    : QObject(parent)
    , m_port(new QSerialPort(this))
    , m_autoDetectTimer(new QTimer(this))
    , m_inSysEx(false)
    , m_state(ConnectionState::Disconnected)
{
    QObject::connect(m_port, &QSerialPort::readyRead,
                     this, &SerialManager::onReadyRead);
    QObject::connect(m_port, &QSerialPort::errorOccurred,
                     this, &SerialManager::onError);
    QObject::connect(m_autoDetectTimer, &QTimer::timeout,
                     this, &SerialManager::onAutoDetectTimer);
}

SerialManager::~SerialManager()
{
    disconnect();
}

QStringList SerialManager::availablePorts() const
{
    QStringList ports;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : infos) {
        QString desc = info.portName();
        if (!info.description().isEmpty()) {
            desc += " - " + info.description();
        }
        ports.append(desc);
    }
    return ports;
}

bool SerialManager::connect(const QString& portName)
{
    if (m_port->isOpen()) {
        disconnect();
    }

    // Extract just the port name (before " - ")
    QString actualPortName = portName.split(" - ").first();

    m_port->setPortName(actualPortName);
    m_port->setBaudRate(BAUD_RATE);
    m_port->setDataBits(QSerialPort::Data8);
    m_port->setParity(QSerialPort::NoParity);
    m_port->setStopBits(QSerialPort::OneStop);
    m_port->setFlowControl(QSerialPort::NoFlowControl);

    m_state = ConnectionState::Connecting;
    emit connectionStateChanged(m_state);

    if (m_port->open(QIODevice::ReadWrite)) {
        m_state = ConnectionState::Connected;
        emit connectionStateChanged(m_state);
        emit connected();
        qDebug() << "Connected to" << actualPortName;

        // Send ping to verify device
        ping();
        return true;
    } else {
        m_state = ConnectionState::Error;
        emit connectionStateChanged(m_state);
        emit connectionError(m_port->errorString());
        qDebug() << "Failed to connect:" << m_port->errorString();
        return false;
    }
}

void SerialManager::disconnect()
{
    m_autoDetectTimer->stop();

    if (m_port->isOpen()) {
        m_port->close();
    }

    m_rxBuffer.clear();
    m_inSysEx = false;
    m_state = ConnectionState::Disconnected;
    emit connectionStateChanged(m_state);
    emit disconnected();
}

bool SerialManager::isConnected() const
{
    return m_port->isOpen();
}

QString SerialManager::connectedPort() const
{
    return m_port->isOpen() ? m_port->portName() : QString();
}

// =============================================================================
// Raw MIDI Messages
// =============================================================================

void SerialManager::sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity)
{
    std::vector<uint8_t> data = {
        static_cast<uint8_t>(0x90 | (channel & 0x0F)),
        static_cast<uint8_t>(note & 0x7F),
        static_cast<uint8_t>(velocity & 0x7F)
    };
    sendRawMIDI(data);
}

void SerialManager::sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity)
{
    std::vector<uint8_t> data = {
        static_cast<uint8_t>(0x80 | (channel & 0x0F)),
        static_cast<uint8_t>(note & 0x7F),
        static_cast<uint8_t>(velocity & 0x7F)
    };
    sendRawMIDI(data);
}

void SerialManager::sendControlChange(uint8_t channel, uint8_t cc, uint8_t value)
{
    std::vector<uint8_t> data = {
        static_cast<uint8_t>(0xB0 | (channel & 0x0F)),
        static_cast<uint8_t>(cc & 0x7F),
        static_cast<uint8_t>(value & 0x7F)
    };
    sendRawMIDI(data);
}

void SerialManager::sendProgramChange(uint8_t channel, uint8_t program)
{
    std::vector<uint8_t> data = {
        static_cast<uint8_t>(0xC0 | (channel & 0x0F)),
        static_cast<uint8_t>(program & 0x7F)
    };
    sendRawMIDI(data);
}

void SerialManager::sendPitchBend(uint8_t channel, uint16_t value)
{
    std::vector<uint8_t> data = {
        static_cast<uint8_t>(0xE0 | (channel & 0x0F)),
        static_cast<uint8_t>(value & 0x7F),         // LSB
        static_cast<uint8_t>((value >> 7) & 0x7F)   // MSB
    };
    sendRawMIDI(data);
}

void SerialManager::sendRawMIDI(const std::vector<uint8_t>& data)
{
    if (!m_port->isOpen() || data.empty()) {
        return;
    }

    QByteArray bytes(reinterpret_cast<const char*>(data.data()),
                     static_cast<int>(data.size()));
    m_port->write(bytes);
}

// =============================================================================
// SysEx Commands
// =============================================================================

void SerialManager::sendSysEx(const std::vector<uint8_t>& data)
{
    if (!m_port->isOpen()) {
        return;
    }

    // Build complete SysEx message: F0 7D 00 <data...> F7
    std::vector<uint8_t> sysex;
    sysex.reserve(data.size() + 4);
    sysex.push_back(0xF0);
    sysex.push_back(SysEx::MANUFACTURER_ID);
    sysex.push_back(SysEx::DEVICE_ID);
    sysex.insert(sysex.end(), data.begin(), data.end());
    sysex.push_back(0xF7);

    sendRawMIDI(sysex);
}

void SerialManager::sendFMPatchToChannel(uint8_t channel, const FMPatch& patch)
{
    if (channel >= 6) return;

    auto patchBytes = patch.toBytes();
    std::vector<uint8_t> data;
    data.reserve(44);
    data.push_back(SysEx::CMD_LOAD_FM_PATCH);
    data.push_back(channel);
    data.insert(data.end(), patchBytes.begin(), patchBytes.end());

    sendSysEx(data);
    qDebug() << "Sent FM patch to channel" << channel;
}

void SerialManager::sendFMPatchToSlot(uint8_t slot, const FMPatch& patch)
{
    if (slot >= 16) return;

    auto patchBytes = patch.toBytes();
    std::vector<uint8_t> data;
    data.reserve(44);
    data.push_back(SysEx::CMD_STORE_FM_PATCH);
    data.push_back(slot);
    data.insert(data.end(), patchBytes.begin(), patchBytes.end());

    sendSysEx(data);
    qDebug() << "Stored FM patch to slot" << slot;
}

void SerialManager::sendPSGEnvelope(uint8_t channel, const PSGEnvelope& env)
{
    if (channel >= 4) return;

    std::vector<uint8_t> data;
    data.reserve(4 + env.length);
    data.push_back(SysEx::CMD_LOAD_PSG_ENV);
    data.push_back(channel);
    data.push_back(env.length);
    data.push_back(env.loopStart);
    for (int i = 0; i < env.length; i++) {
        data.push_back(env.data[i]);
    }

    sendSysEx(data);
    qDebug() << "Sent PSG envelope to channel" << channel;
}

void SerialManager::recallPatchToChannel(uint8_t channel, uint8_t slot)
{
    if (channel >= 6 || slot >= 16) return;

    std::vector<uint8_t> data = {
        SysEx::CMD_RECALL_PATCH,
        channel,
        slot
    };
    sendSysEx(data);
    qDebug() << "Recalled slot" << slot << "to channel" << channel;
}

void SerialManager::requestPatchDump(uint8_t slot)
{
    if (slot >= 16) return;

    std::vector<uint8_t> data = {
        SysEx::CMD_REQUEST_PATCH,
        slot
    };
    sendSysEx(data);
}

void SerialManager::requestAllPatches()
{
    std::vector<uint8_t> data = {
        SysEx::CMD_REQUEST_ALL
    };
    sendSysEx(data);
}

void SerialManager::setSynthMode(SynthMode mode)
{
    std::vector<uint8_t> data = {
        SysEx::CMD_SET_MODE,
        static_cast<uint8_t>(mode)
    };
    sendSysEx(data);
    qDebug() << "Set synth mode to" << (mode == SynthMode::Poly ? "Poly" : "Multi");
}

void SerialManager::ping()
{
    std::vector<uint8_t> data = {
        SysEx::CMD_PING
    };
    sendSysEx(data);
}

// =============================================================================
// Receive Handling
// =============================================================================

void SerialManager::onReadyRead()
{
    QByteArray data = m_port->readAll();

    for (char c : data) {
        uint8_t byte = static_cast<uint8_t>(c);

        if (byte == 0xF0) {
            // Start of SysEx
            m_inSysEx = true;
            m_rxBuffer.clear();
            m_rxBuffer.append(c);
        } else if (byte == 0xF7 && m_inSysEx) {
            // End of SysEx
            m_rxBuffer.append(c);
            processSysEx(m_rxBuffer);
            m_rxBuffer.clear();
            m_inSysEx = false;
        } else if (m_inSysEx) {
            // Middle of SysEx
            m_rxBuffer.append(c);
        } else {
            // Non-SysEx MIDI data (could be debug output or echoed MIDI)
            emit midiDataReceived(QByteArray(1, c));
        }
    }
}

void SerialManager::processSysEx(const QByteArray& sysex)
{
    // Minimum: F0 7D 00 cmd F7 = 5 bytes
    if (sysex.size() < 5) return;

    // Check manufacturer ID
    if (static_cast<uint8_t>(sysex[1]) != SysEx::MANUFACTURER_ID) return;

    uint8_t cmd = static_cast<uint8_t>(sysex[3]);

    switch (cmd) {
        case SysEx::RESP_PATCH_DUMP:
            // F0 7D 00 80 <slot> <42 bytes> F7
            if (sysex.size() >= 5 + 1 + 42) {
                uint8_t slot = static_cast<uint8_t>(sysex[4]);
                FMPatch patch = FMPatch::fromBytes(
                    reinterpret_cast<const uint8_t*>(sysex.constData() + 5));
                emit patchReceived(slot, patch);
                qDebug() << "Received patch dump for slot" << slot;
            }
            break;

        case SysEx::RESP_IDENTITY:
            // F0 7D 00 81 <mode> <version> F7
            if (sysex.size() >= 5 + 2) {
                uint8_t mode = static_cast<uint8_t>(sysex[4]);
                uint8_t version = static_cast<uint8_t>(sysex[5]);
                emit identityReceived(mode, version);
                qDebug() << "Device identified: mode=" << mode << "version=" << version;
            }
            break;

        default:
            qDebug() << "Unknown SysEx response:" << Qt::hex << cmd;
            break;
    }
}

void SerialManager::onError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return;
    }

    qDebug() << "Serial error:" << m_port->errorString();

    if (error == QSerialPort::ResourceError) {
        // Device disconnected
        disconnect();
    }

    m_state = ConnectionState::Error;
    emit connectionStateChanged(m_state);
    emit connectionError(m_port->errorString());
}

void SerialManager::onAutoDetectTimer()
{
    // Auto-detect Arduino/Teensy ports
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : ports) {
        if (isArduinoPort(info)) {
            qDebug() << "Auto-detected Arduino/Teensy on" << info.portName();
            connect(info.portName());
            break;
        }
    }
}

bool SerialManager::isArduinoPort(const QSerialPortInfo& info) const
{
    // Common Arduino/Teensy VID/PID combinations
    static const QList<QPair<quint16, quint16>> knownDevices = {
        {0x2341, 0x0043},  // Arduino Uno
        {0x2341, 0x0001},  // Arduino Uno (older)
        {0x2341, 0x0010},  // Arduino Mega
        {0x2341, 0x003D},  // Arduino Due
        {0x1A86, 0x7523},  // CH340 (cheap clones)
        {0x0403, 0x6001},  // FTDI FT232
        {0x16C0, 0x0483},  // Teensy (Serial)
        {0x16C0, 0x0489},  // Teensy (Serial + MIDI)
    };

    quint16 vid = info.vendorIdentifier();
    quint16 pid = info.productIdentifier();

    for (const auto& device : knownDevices) {
        if (vid == device.first && pid == device.second) {
            return true;
        }
    }

    // Also check description for common strings
    QString desc = info.description().toLower();
    return desc.contains("arduino") || desc.contains("teensy") ||
           desc.contains("ch340") || desc.contains("ftdi");
}

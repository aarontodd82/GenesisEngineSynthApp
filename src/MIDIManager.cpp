#include "MIDIManager.h"
#include <QDebug>
#include <QTimer>

// =============================================================================
// Platform-specific includes and implementation
// =============================================================================

#if defined(USE_COREMIDI)
    #include <CoreMIDI/CoreMIDI.h>
    #include <CoreFoundation/CoreFoundation.h>
#elif defined(USE_ALSA)
    #include <alsa/asoundlib.h>
#elif defined(USE_RTMIDI)
    #include <rtmidi/RtMidi.h>
#elif defined(USE_WINMM)
    #include <windows.h>
    #include <mmsystem.h>
#endif

// =============================================================================
// Private Implementation
// =============================================================================

class MIDIManagerPrivate {
public:
    MIDIManager* q;

#if defined(USE_COREMIDI)
    MIDIClientRef client = 0;
    MIDIPortRef inputPort = 0;
    MIDIEndpointRef virtualSource = 0;
    MIDIEndpointRef connectedSource = 0;

    static void midiReadProc(const MIDIPacketList* pktList, void* readProcRefCon, void* srcConnRefCon) {
        auto* d = static_cast<MIDIManagerPrivate*>(readProcRefCon);
        const MIDIPacket* packet = &pktList->packet[0];
        for (UInt32 i = 0; i < pktList->numPackets; i++) {
            std::vector<uint8_t> data(packet->data, packet->data + packet->length);
            d->processMessage(data);
            packet = MIDIPacketNext(packet);
        }
    }

#elif defined(USE_ALSA)
    snd_seq_t* seq = nullptr;
    int clientId = -1;
    int inputPortId = -1;
    int virtualPortId = -1;
    QTimer* pollTimer = nullptr;

#elif defined(USE_RTMIDI)
    RtMidiIn* midiIn = nullptr;

    static void midiCallback(double timeStamp, std::vector<unsigned char>* message, void* userData) {
        Q_UNUSED(timeStamp);
        auto* d = static_cast<MIDIManagerPrivate*>(userData);
        if (message && !message->empty()) {
            std::vector<uint8_t> data(message->begin(), message->end());
            d->processMessage(data);
        }
    }

#elif defined(USE_WINMM)
    HMIDIIN midiIn = nullptr;
    std::vector<uint8_t> sysExBuffer;

    static void CALLBACK midiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance,
                                    DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
        Q_UNUSED(hMidiIn);
        Q_UNUSED(dwParam2);
        auto* d = reinterpret_cast<MIDIManagerPrivate*>(dwInstance);
        if (wMsg == MIM_DATA) {
            // Short message
            std::vector<uint8_t> data;
            data.push_back(dwParam1 & 0xFF);
            data.push_back((dwParam1 >> 8) & 0xFF);
            data.push_back((dwParam1 >> 16) & 0xFF);
            d->processMessage(data);
        }
    }
#endif

    QString currentPortName;
    bool hasVirtual = false;

    void processMessage(const std::vector<uint8_t>& data) {
        if (data.empty()) return;

        // Emit raw data for forwarding
        emit q->midiReceived(data);

        // Parse and emit typed events
        uint8_t status = data[0];
        uint8_t type = status & 0xF0;
        uint8_t channel = status & 0x0F;

        switch (type) {
            case 0x90:  // Note On
                if (data.size() >= 3) {
                    if (data[2] > 0) {
                        emit q->noteOnReceived(channel, data[1], data[2]);
                    } else {
                        emit q->noteOffReceived(channel, data[1], 0);
                    }
                }
                break;
            case 0x80:  // Note Off
                if (data.size() >= 3) {
                    emit q->noteOffReceived(channel, data[1], data[2]);
                }
                break;
            case 0xB0:  // Control Change
                if (data.size() >= 3) {
                    emit q->controlChangeReceived(channel, data[1], data[2]);
                }
                break;
            case 0xC0:  // Program Change
                if (data.size() >= 2) {
                    emit q->programChangeReceived(channel, data[1]);
                }
                break;
            case 0xE0:  // Pitch Bend
                if (data.size() >= 3) {
                    uint16_t value = data[1] | (data[2] << 7);
                    emit q->pitchBendReceived(channel, value);
                }
                break;
            case 0xF0:  // System messages
                if (status == 0xF0) {
                    emit q->sysExReceived(data);
                }
                break;
        }
    }
};

// =============================================================================
// MIDIManager Implementation
// =============================================================================

MIDIManager::MIDIManager(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<MIDIManagerPrivate>())
    , m_forwardingEnabled(true)
{
    d->q = this;

#if defined(USE_COREMIDI)
    CFStringRef clientName = CFSTR("GenesisEngineSynth");
    MIDIClientCreate(clientName, nullptr, nullptr, &d->client);

    CFStringRef portName = CFSTR("Input");
    MIDIInputPortCreate(d->client, portName, MIDIManagerPrivate::midiReadProc, d.get(), &d->inputPort);

#elif defined(USE_ALSA)
    if (snd_seq_open(&d->seq, "default", SND_SEQ_OPEN_DUPLEX, 0) >= 0) {
        snd_seq_set_client_name(d->seq, "GenesisEngineSynth");
        d->clientId = snd_seq_client_id(d->seq);

        // Create input port
        d->inputPortId = snd_seq_create_simple_port(d->seq, "Input",
            SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
            SND_SEQ_PORT_TYPE_APPLICATION);

        // Poll timer for ALSA events
        d->pollTimer = new QTimer(this);
        connect(d->pollTimer, &QTimer::timeout, this, [this]() {
            snd_seq_event_t* ev;
            while (snd_seq_event_input_pending(d->seq, 1) > 0) {
                snd_seq_event_input(d->seq, &ev);
                // Convert ALSA event to MIDI bytes
                std::vector<uint8_t> data;
                switch (ev->type) {
                    case SND_SEQ_EVENT_NOTEON:
                        data = {static_cast<uint8_t>(0x90 | ev->data.note.channel),
                                ev->data.note.note, ev->data.note.velocity};
                        break;
                    case SND_SEQ_EVENT_NOTEOFF:
                        data = {static_cast<uint8_t>(0x80 | ev->data.note.channel),
                                ev->data.note.note, ev->data.note.velocity};
                        break;
                    case SND_SEQ_EVENT_CONTROLLER:
                        data = {static_cast<uint8_t>(0xB0 | ev->data.control.channel),
                                static_cast<uint8_t>(ev->data.control.param),
                                static_cast<uint8_t>(ev->data.control.value)};
                        break;
                    case SND_SEQ_EVENT_PGMCHANGE:
                        data = {static_cast<uint8_t>(0xC0 | ev->data.control.channel),
                                static_cast<uint8_t>(ev->data.control.value)};
                        break;
                    case SND_SEQ_EVENT_PITCHBEND:
                        {
                            int bend = ev->data.control.value + 8192;
                            data = {static_cast<uint8_t>(0xE0 | ev->data.control.channel),
                                    static_cast<uint8_t>(bend & 0x7F),
                                    static_cast<uint8_t>((bend >> 7) & 0x7F)};
                        }
                        break;
                }
                if (!data.empty()) {
                    d->processMessage(data);
                }
                snd_seq_free_event(ev);
            }
        });
        d->pollTimer->start(1);  // 1ms polling
    }

#elif defined(USE_RTMIDI)
    try {
        d->midiIn = new RtMidiIn();
        d->midiIn->setCallback(MIDIManagerPrivate::midiCallback, d.get());
        d->midiIn->ignoreTypes(false, false, false);  // Don't ignore SysEx, timing, sensing
    } catch (RtMidiError& error) {
        qWarning() << "RtMidi error:" << error.getMessage().c_str();
    }

#elif defined(USE_WINMM)
    // WinMM doesn't need initialization
#endif
}

MIDIManager::~MIDIManager()
{
    closeInputPort();
    destroyVirtualInputPort();

#if defined(USE_COREMIDI)
    if (d->inputPort) MIDIPortDispose(d->inputPort);
    if (d->client) MIDIClientDispose(d->client);

#elif defined(USE_ALSA)
    if (d->pollTimer) d->pollTimer->stop();
    if (d->seq) snd_seq_close(d->seq);

#elif defined(USE_RTMIDI)
    delete d->midiIn;

#elif defined(USE_WINMM)
    // Cleanup handled by closeInputPort
#endif
}

QStringList MIDIManager::availableInputPorts() const
{
    QStringList ports;

#if defined(USE_COREMIDI)
    ItemCount numSources = MIDIGetNumberOfSources();
    for (ItemCount i = 0; i < numSources; i++) {
        MIDIEndpointRef src = MIDIGetSource(i);
        CFStringRef name;
        MIDIObjectGetStringProperty(src, kMIDIPropertyDisplayName, &name);
        if (name) {
            char buffer[256];
            CFStringGetCString(name, buffer, sizeof(buffer), kCFStringEncodingUTF8);
            ports.append(QString::fromUtf8(buffer));
            CFRelease(name);
        }
    }

#elif defined(USE_ALSA)
    snd_seq_client_info_t* cinfo;
    snd_seq_port_info_t* pinfo;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);

    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(d->seq, cinfo) >= 0) {
        int client = snd_seq_client_info_get_client(cinfo);
        if (client == d->clientId) continue;  // Skip ourselves

        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(d->seq, pinfo) >= 0) {
            unsigned int caps = snd_seq_port_info_get_capability(pinfo);
            if ((caps & SND_SEQ_PORT_CAP_READ) && (caps & SND_SEQ_PORT_CAP_SUBS_READ)) {
                QString name = QString("%1:%2 - %3")
                    .arg(client)
                    .arg(snd_seq_port_info_get_port(pinfo))
                    .arg(snd_seq_port_info_get_name(pinfo));
                ports.append(name);
            }
        }
    }

#elif defined(USE_RTMIDI)
    if (d->midiIn) {
        unsigned int count = d->midiIn->getPortCount();
        for (unsigned int i = 0; i < count; i++) {
            ports.append(QString::fromStdString(d->midiIn->getPortName(i)));
        }
    }

#elif defined(USE_WINMM)
    UINT numDevs = midiInGetNumDevs();
    for (UINT i = 0; i < numDevs; i++) {
        MIDIINCAPS caps;
        if (midiInGetDevCaps(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
            ports.append(QString::fromWCharArray(caps.szPname));
        }
    }
#endif

    return ports;
}

void MIDIManager::refreshPorts()
{
    emit portsChanged();
}

bool MIDIManager::openInputPort(int portIndex)
{
    closeInputPort();

#if defined(USE_COREMIDI)
    if (portIndex >= 0 && portIndex < (int)MIDIGetNumberOfSources()) {
        d->connectedSource = MIDIGetSource(portIndex);
        MIDIPortConnectSource(d->inputPort, d->connectedSource, nullptr);

        CFStringRef name;
        MIDIObjectGetStringProperty(d->connectedSource, kMIDIPropertyDisplayName, &name);
        if (name) {
            char buffer[256];
            CFStringGetCString(name, buffer, sizeof(buffer), kCFStringEncodingUTF8);
            d->currentPortName = QString::fromUtf8(buffer);
            CFRelease(name);
        }
        emit inputOpened(d->currentPortName);
        return true;
    }

#elif defined(USE_ALSA)
    // ALSA uses subscription model - port is always "open" via virtual port
    // For explicit connection, we'd parse the portIndex back to client:port
    d->currentPortName = availableInputPorts().value(portIndex);
    emit inputOpened(d->currentPortName);
    return true;

#elif defined(USE_RTMIDI)
    if (d->midiIn && portIndex >= 0 && portIndex < (int)d->midiIn->getPortCount()) {
        try {
            d->midiIn->openPort(portIndex);
            d->currentPortName = QString::fromStdString(d->midiIn->getPortName(portIndex));
            emit inputOpened(d->currentPortName);
            return true;
        } catch (RtMidiError& error) {
            emit this->error(QString::fromStdString(error.getMessage()));
        }
    }

#elif defined(USE_WINMM)
    MMRESULT result = midiInOpen(&d->midiIn, portIndex,
                                  (DWORD_PTR)MIDIManagerPrivate::midiInProc,
                                  (DWORD_PTR)d.get(), CALLBACK_FUNCTION);
    if (result == MMSYSERR_NOERROR) {
        midiInStart(d->midiIn);
        MIDIINCAPS caps;
        midiInGetDevCaps(portIndex, &caps, sizeof(caps));
        d->currentPortName = QString::fromWCharArray(caps.szPname);
        emit inputOpened(d->currentPortName);
        return true;
    }
#endif

    return false;
}

bool MIDIManager::openInputPort(const QString& portName)
{
    QStringList ports = availableInputPorts();
    int index = ports.indexOf(portName);
    if (index >= 0) {
        return openInputPort(index);
    }

    // Try partial match
    for (int i = 0; i < ports.size(); i++) {
        if (ports[i].contains(portName, Qt::CaseInsensitive)) {
            return openInputPort(i);
        }
    }

    return false;
}

void MIDIManager::closeInputPort()
{
#if defined(USE_COREMIDI)
    if (d->connectedSource) {
        MIDIPortDisconnectSource(d->inputPort, d->connectedSource);
        d->connectedSource = 0;
    }

#elif defined(USE_ALSA)
    // Virtual port stays open

#elif defined(USE_RTMIDI)
    if (d->midiIn && d->midiIn->isPortOpen()) {
        d->midiIn->closePort();
    }

#elif defined(USE_WINMM)
    if (d->midiIn) {
        midiInStop(d->midiIn);
        midiInClose(d->midiIn);
        d->midiIn = nullptr;
    }
#endif

    d->currentPortName.clear();
    emit inputClosed();
}

bool MIDIManager::isInputOpen() const
{
#if defined(USE_COREMIDI)
    return d->connectedSource != 0;
#elif defined(USE_ALSA)
    return d->inputPortId >= 0;
#elif defined(USE_RTMIDI)
    return d->midiIn && d->midiIn->isPortOpen();
#elif defined(USE_WINMM)
    return d->midiIn != nullptr;
#endif
    return false;
}

QString MIDIManager::currentInputPort() const
{
    return d->currentPortName;
}

bool MIDIManager::canCreateVirtualPorts() const
{
#if defined(USE_COREMIDI) || defined(USE_ALSA)
    return true;
#else
    return false;
#endif
}

bool MIDIManager::createVirtualInputPort(const QString& name)
{
    if (!canCreateVirtualPorts()) {
        return false;
    }

#if defined(USE_COREMIDI)
    if (d->virtualSource) {
        return true;  // Already created
    }

    CFStringRef cfName = CFStringCreateWithCString(nullptr, name.toUtf8().constData(),
                                                    kCFStringEncodingUTF8);
    OSStatus status = MIDISourceCreate(d->client, cfName, &d->virtualSource);
    CFRelease(cfName);

    if (status == noErr) {
        d->hasVirtual = true;
        qDebug() << "Created virtual MIDI source:" << name;
        return true;
    }

#elif defined(USE_ALSA)
    if (d->virtualPortId >= 0) {
        return true;  // Already created
    }

    d->virtualPortId = snd_seq_create_simple_port(d->seq, name.toUtf8().constData(),
        SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);

    if (d->virtualPortId >= 0) {
        d->hasVirtual = true;
        qDebug() << "Created virtual MIDI port:" << name;
        return true;
    }
#endif

    return false;
}

void MIDIManager::destroyVirtualInputPort()
{
#if defined(USE_COREMIDI)
    if (d->virtualSource) {
        MIDIEndpointDispose(d->virtualSource);
        d->virtualSource = 0;
        d->hasVirtual = false;
    }

#elif defined(USE_ALSA)
    if (d->virtualPortId >= 0) {
        snd_seq_delete_simple_port(d->seq, d->virtualPortId);
        d->virtualPortId = -1;
        d->hasVirtual = false;
    }
#endif
}

bool MIDIManager::hasVirtualPort() const
{
    return d->hasVirtual;
}

void MIDIManager::setForwardingEnabled(bool enabled)
{
    m_forwardingEnabled = enabled;
}

bool MIDIManager::isForwardingEnabled() const
{
    return m_forwardingEnabled;
}

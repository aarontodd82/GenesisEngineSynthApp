#include "MainWindow.h"
#include "SerialManager.h"
#include "MIDIManager.h"
#include "PatchBank.h"
#include "FileFormats.h"
#include "FMPatchEditor.h"
#include "PSGEnvelopeEditor.h"
#include "PianoKeyboardWidget.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QTabWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QApplication>
#include <QRandomGenerator>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_serial(new SerialManager(this))
    , m_midi(new MIDIManager(this))
    , m_patchBank(new PatchBank(this))
    , m_midiRxTimer(new QTimer(this))
    , m_midiTxTimer(new QTimer(this))
{
    setWindowTitle("Genesis Engine Synth");
    setMinimumSize(1000, 700);

    // Setup LED timers (single-shot, 100ms flash duration)
    m_midiRxTimer->setSingleShot(true);
    m_midiRxTimer->setInterval(100);
    m_midiTxTimer->setSingleShot(true);
    m_midiTxTimer->setInterval(100);

    setupUI();
    setupMenus();
    setupConnections();
    loadSettings();

    refreshSerialPorts();
    refreshMIDIPorts();
    updatePatchList();

    statusBar()->showMessage("Ready");
}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::setupUI()
{
    // Central widget with splitter
    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter);

    // Left panel: Connection and patch bank
    QWidget* leftPanel = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(4, 4, 4, 4);

    // Connection group
    QGroupBox* connectionGroup = new QGroupBox("Connection");
    QVBoxLayout* connLayout = new QVBoxLayout(connectionGroup);

    QHBoxLayout* serialRow = new QHBoxLayout();
    m_serialPortCombo = new QComboBox();
    m_serialPortCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_refreshButton = new QPushButton("Refresh");
    m_connectButton = new QPushButton("Connect");
    serialRow->addWidget(m_serialPortCombo);
    serialRow->addWidget(m_refreshButton);
    serialRow->addWidget(m_connectButton);
    connLayout->addLayout(serialRow);

    m_connectionStatus = new QLabel("Disconnected");
    m_connectionStatus->setStyleSheet("color: #888;");
    connLayout->addWidget(m_connectionStatus);

    // Board info label (hidden until connected)
    m_boardInfoLabel = new QLabel();
    m_boardInfoLabel->setWordWrap(true);
    m_boardInfoLabel->setStyleSheet("color: #aaa; font-size: 11px; padding: 4px; background-color: #2a2a2a; border-radius: 3px;");
    m_boardInfoLabel->hide();
    connLayout->addWidget(m_boardInfoLabel);

    leftLayout->addWidget(connectionGroup);

    // MIDI group
    QGroupBox* midiGroup = new QGroupBox("MIDI Input");
    QVBoxLayout* midiLayout = new QVBoxLayout(midiGroup);

    m_midiPortCombo = new QComboBox();
    m_midiPortCombo->addItem("(None)");
    midiLayout->addWidget(m_midiPortCombo);

    QHBoxLayout* midiRow = new QHBoxLayout();
    m_virtualMidiButton = new QPushButton("Create Virtual Port");
    m_midiForwardCheck = new QCheckBox("Forward to device");
    m_midiForwardCheck->setChecked(true);
    midiRow->addWidget(m_virtualMidiButton);
    midiRow->addWidget(m_midiForwardCheck);
    midiLayout->addLayout(midiRow);

    // MIDI activity LEDs
    QHBoxLayout* midiLedRow = new QHBoxLayout();
    midiLedRow->addWidget(new QLabel("Activity:"));

    m_midiRxLed = new QLabel("RX");
    m_midiRxLed->setFixedSize(28, 18);
    m_midiRxLed->setAlignment(Qt::AlignCenter);
    m_midiRxLed->setStyleSheet("background-color: #333; color: #666; border: 1px solid #555; border-radius: 3px; font-size: 10px;");
    midiLedRow->addWidget(m_midiRxLed);

    m_midiTxLed = new QLabel("TX");
    m_midiTxLed->setFixedSize(28, 18);
    m_midiTxLed->setAlignment(Qt::AlignCenter);
    m_midiTxLed->setStyleSheet("background-color: #333; color: #666; border: 1px solid #555; border-radius: 3px; font-size: 10px;");
    midiLedRow->addWidget(m_midiTxLed);

    midiLedRow->addStretch();
    midiLayout->addLayout(midiLedRow);

    // Disable virtual port button on Windows
    if (!m_midi->canCreateVirtualPorts()) {
        m_virtualMidiButton->setEnabled(false);
        m_virtualMidiButton->setToolTip("Virtual ports require loopMIDI on Windows");
    }

    leftLayout->addWidget(midiGroup);

    // Mode group
    QGroupBox* modeGroup = new QGroupBox("Synth Mode");
    QHBoxLayout* modeLayout = new QHBoxLayout(modeGroup);
    m_modeCombo = new QComboBox();
    m_modeCombo->addItem("Multi-timbral (6 channels)");
    m_modeCombo->addItem("Poly (6-voice on Ch 1)");
    modeLayout->addWidget(m_modeCombo);
    leftLayout->addWidget(modeGroup);

    // FM Patch Bank
    QGroupBox* fmBankGroup = new QGroupBox("FM Patches");
    QVBoxLayout* fmBankLayout = new QVBoxLayout(fmBankGroup);

    m_fmPatchList = new QListWidget();
    m_fmPatchList->setMaximumHeight(200);
    fmBankLayout->addWidget(m_fmPatchList);

    QHBoxLayout* fmButtonRow = new QHBoxLayout();
    m_loadPatchButton = new QPushButton("Load...");
    m_savePatchButton = new QPushButton("Save...");
    m_sendPatchButton = new QPushButton("Send to Device");
    fmButtonRow->addWidget(m_loadPatchButton);
    fmButtonRow->addWidget(m_savePatchButton);
    fmButtonRow->addWidget(m_sendPatchButton);
    fmBankLayout->addLayout(fmButtonRow);

    // Live Edit checkbox
    m_liveEditCheck = new QCheckBox("Live Edit (auto-send on change)");
    m_liveEditCheck->setToolTip("When enabled, patch changes are sent to the device in real-time");
    fmBankLayout->addWidget(m_liveEditCheck);

    // Randomize button
    m_randomizeButton = new QPushButton("Randomize Patch");
    m_randomizeButton->setToolTip("Generate a random FM patch with sensible constraints");
    fmBankLayout->addWidget(m_randomizeButton);

    // Target selection
    QHBoxLayout* targetRow = new QHBoxLayout();
    targetRow->addWidget(new QLabel("Channel:"));
    m_targetChannel = new QSpinBox();
    m_targetChannel->setRange(1, 6);
    m_targetChannel->setValue(1);
    targetRow->addWidget(m_targetChannel);
    targetRow->addWidget(new QLabel("Slot:"));
    m_targetSlot = new QSpinBox();
    m_targetSlot->setRange(0, 15);
    m_targetSlot->setValue(0);
    targetRow->addWidget(m_targetSlot);
    targetRow->addStretch();
    fmBankLayout->addLayout(targetRow);

    // Channel controls (Pan, LFO)
    QGroupBox* channelCtrlGroup = new QGroupBox("Channel Controls");
    QGridLayout* channelCtrlLayout = new QGridLayout(channelCtrlGroup);

    channelCtrlLayout->addWidget(new QLabel("Pan:"), 0, 0);
    m_panCombo = new QComboBox();
    m_panCombo->addItems({"Left", "Center", "Right"});
    m_panCombo->setCurrentIndex(1);  // Default to center
    channelCtrlLayout->addWidget(m_panCombo, 0, 1);

    m_lfoEnableCheck = new QCheckBox("LFO");
    m_lfoEnableCheck->setToolTip("Enable vibrato/tremolo LFO");
    channelCtrlLayout->addWidget(m_lfoEnableCheck, 1, 0);

    m_lfoSpeedCombo = new QComboBox();
    m_lfoSpeedCombo->addItems({"3.98 Hz", "5.56 Hz", "6.02 Hz", "6.37 Hz",
                               "6.88 Hz", "9.63 Hz", "48.1 Hz", "72.2 Hz"});
    m_lfoSpeedCombo->setCurrentIndex(1);  // ~5.56 Hz default
    m_lfoSpeedCombo->setEnabled(false);
    channelCtrlLayout->addWidget(m_lfoSpeedCombo, 1, 1);

    fmBankLayout->addWidget(channelCtrlGroup);

    leftLayout->addWidget(fmBankGroup);

    // PSG Envelope Bank
    QGroupBox* psgBankGroup = new QGroupBox("PSG Envelopes");
    QVBoxLayout* psgBankLayout = new QVBoxLayout(psgBankGroup);

    m_psgEnvList = new QListWidget();
    m_psgEnvList->setMaximumHeight(120);
    psgBankLayout->addWidget(m_psgEnvList);

    leftLayout->addWidget(psgBankGroup);
    leftLayout->addStretch();

    // Right panel: Editors + Keyboard
    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(4);

    QTabWidget* editorTabs = new QTabWidget();

    m_fmEditor = new FMPatchEditor();
    editorTabs->addTab(m_fmEditor, "FM Patch Editor");

    m_psgEditor = new PSGEnvelopeEditor();
    editorTabs->addTab(m_psgEditor, "PSG Envelope Editor");

    rightLayout->addWidget(editorTabs, 1);

    // On-screen keyboard
    QGroupBox* keyboardGroup = new QGroupBox("Keyboard");
    QVBoxLayout* keyboardLayout = new QVBoxLayout(keyboardGroup);
    keyboardLayout->setContentsMargins(4, 4, 4, 4);

    m_keyboard = new PianoKeyboardWidget();
    m_keyboard->setNumOctaves(3);
    m_keyboard->setBaseOctave(3);
    keyboardLayout->addWidget(m_keyboard);

    QHBoxLayout* keyboardControls = new QHBoxLayout();
    keyboardControls->addWidget(new QLabel("Octave:"));
    QSpinBox* octaveSpin = new QSpinBox();
    octaveSpin->setRange(0, 7);
    octaveSpin->setValue(3);
    connect(octaveSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            m_keyboard, &PianoKeyboardWidget::setBaseOctave);
    keyboardControls->addWidget(octaveSpin);

    keyboardControls->addWidget(new QLabel("Velocity:"));
    QSpinBox* velocitySpin = new QSpinBox();
    velocitySpin->setRange(1, 127);
    velocitySpin->setValue(100);
    connect(velocitySpin, QOverload<int>::of(&QSpinBox::valueChanged),
            m_keyboard, &PianoKeyboardWidget::setVelocity);
    keyboardControls->addWidget(velocitySpin);

    keyboardControls->addStretch();

    // Panic button
    m_panicButton = new QPushButton("PANIC");
    m_panicButton->setToolTip("Send All Notes Off to all channels (stops stuck notes)");
    m_panicButton->setStyleSheet(
        "QPushButton { background-color: #600; color: #fff; font-weight: bold; padding: 4px 12px; }"
        "QPushButton:hover { background-color: #800; }"
        "QPushButton:pressed { background-color: #a00; }");
    keyboardControls->addWidget(m_panicButton);

    keyboardLayout->addLayout(keyboardControls);

    rightLayout->addWidget(keyboardGroup);

    // Add to splitter
    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setSizes({280, 720});
}

void MainWindow::setupMenus()
{
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");

    QAction* newAction = fileMenu->addAction("&New Bank", this, &MainWindow::onNewBank);
    newAction->setShortcut(QKeySequence::New);

    QAction* openAction = fileMenu->addAction("&Open Bank...", this, &MainWindow::onOpenBank);
    openAction->setShortcut(QKeySequence::Open);

    QAction* saveAction = fileMenu->addAction("&Save Bank", this, &MainWindow::onSaveBank);
    saveAction->setShortcut(QKeySequence::Save);

    fileMenu->addAction("Save Bank &As...", this, &MainWindow::onSaveBankAs);

    fileMenu->addSeparator();

    QAction* quitAction = fileMenu->addAction("&Quit", this, &QWidget::close);
    quitAction->setShortcut(QKeySequence::Quit);

    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About", this, [this]() {
        QMessageBox::about(this, "About Genesis Engine Synth",
            "Genesis Engine Synth Companion\n\n"
            "Version 1.0\n\n"
            "A companion application for the GenesisEngine MIDISynth.\n"
            "Provides MIDI bridging, patch editing, and bank management.");
    });
}

void MainWindow::setupConnections()
{
    // Serial connections
    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshPortsClicked);
    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_serial, &SerialManager::connected, this, &MainWindow::onSerialConnected);
    connect(m_serial, &SerialManager::disconnected, this, &MainWindow::onSerialDisconnected);
    connect(m_serial, &SerialManager::connectionError, this, &MainWindow::onSerialError);
    connect(m_serial, &SerialManager::boardTypeDetected, this, &MainWindow::onBoardTypeDetected);
    connect(m_serial, &SerialManager::ccReceived, this, &MainWindow::onCCReceived);

    // MIDI connections
    connect(m_midiPortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onMIDIPortChanged);
    connect(m_virtualMidiButton, &QPushButton::clicked, this, &MainWindow::onCreateVirtualPort);
    connect(m_midi, &MIDIManager::midiReceived, this, &MainWindow::onMIDIReceived);
    connect(m_midiForwardCheck, &QCheckBox::toggled, m_midi, &MIDIManager::setForwardingEnabled);

    // Patch bank connections
    connect(m_fmPatchList, &QListWidget::currentRowChanged, this, &MainWindow::onFMPatchSelected);
    connect(m_psgEnvList, &QListWidget::currentRowChanged, this, &MainWindow::onPSGEnvelopeSelected);
    connect(m_loadPatchButton, &QPushButton::clicked, this, &MainWindow::onLoadPatchClicked);
    connect(m_savePatchButton, &QPushButton::clicked, this, &MainWindow::onSavePatchClicked);
    connect(m_sendPatchButton, &QPushButton::clicked, this, &MainWindow::onSendPatchClicked);
    connect(m_randomizeButton, &QPushButton::clicked, this, &MainWindow::onRandomizePatchClicked);

    // Editor connections
    connect(m_fmEditor, &FMPatchEditor::patchChanged, this, &MainWindow::onPatchEdited);

    // Mode
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onModeChanged);

    // Channel controls
    connect(m_panCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onPanChanged);
    connect(m_lfoEnableCheck, &QCheckBox::toggled, this, &MainWindow::onLfoEnableChanged);
    connect(m_lfoSpeedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onLfoSpeedChanged);

    // Keyboard
    connect(m_keyboard, &PianoKeyboardWidget::noteOn, this, &MainWindow::onKeyboardNoteOn);
    connect(m_keyboard, &PianoKeyboardWidget::noteOff, this, &MainWindow::onKeyboardNoteOff);
    connect(m_panicButton, &QPushButton::clicked, this, &MainWindow::onPanicClicked);

    // MIDI activity LED timers
    connect(m_midiRxTimer, &QTimer::timeout, this, &MainWindow::onMidiRxLedTimeout);
    connect(m_midiTxTimer, &QTimer::timeout, this, &MainWindow::onMidiTxLedTimeout);
}

void MainWindow::refreshSerialPorts()
{
    QString currentPort = m_serialPortCombo->currentText();
    m_serialPortCombo->clear();

    QStringList ports = m_serial->availablePorts();
    m_serialPortCombo->addItems(ports);

    // Try to restore previous selection
    int idx = m_serialPortCombo->findText(currentPort);
    if (idx >= 0) {
        m_serialPortCombo->setCurrentIndex(idx);
    }
}

void MainWindow::refreshMIDIPorts()
{
    QString currentPort = m_midiPortCombo->currentText();
    m_midiPortCombo->clear();
    m_midiPortCombo->addItem("(None)");

    QStringList ports = m_midi->availableInputPorts();
    m_midiPortCombo->addItems(ports);

    // Try to restore previous selection
    int idx = m_midiPortCombo->findText(currentPort);
    if (idx >= 0) {
        m_midiPortCombo->setCurrentIndex(idx);
    }
}

void MainWindow::updateConnectionStatus()
{
    if (m_serial->isConnected()) {
        m_connectionStatus->setText("Connected: " + m_serial->connectedPort());
        m_connectionStatus->setStyleSheet("color: #0a0;");
        m_connectButton->setText("Disconnect");
    } else {
        m_connectionStatus->setText("Disconnected");
        m_connectionStatus->setStyleSheet("color: #888;");
        m_connectButton->setText("Connect");
    }
}

void MainWindow::updatePatchList()
{
    // FM patches
    m_fmPatchList->clear();
    for (int i = 0; i < PatchBank::FM_SLOT_COUNT; i++) {
        m_fmPatchList->addItem(QString("%1: %2").arg(i).arg(m_patchBank->fmPatchName(i)));
    }
    if (m_selectedFMSlot >= 0 && m_selectedFMSlot < PatchBank::FM_SLOT_COUNT) {
        m_fmPatchList->setCurrentRow(m_selectedFMSlot);
    }

    // PSG envelopes
    m_psgEnvList->clear();
    for (int i = 0; i < PatchBank::PSG_SLOT_COUNT; i++) {
        m_psgEnvList->addItem(QString("%1: %2").arg(i).arg(m_patchBank->psgEnvelopeName(i)));
    }
    if (m_selectedPSGSlot >= 0 && m_selectedPSGSlot < PatchBank::PSG_SLOT_COUNT) {
        m_psgEnvList->setCurrentRow(m_selectedPSGSlot);
    }
}

void MainWindow::onConnectClicked()
{
    if (m_serial->isConnected()) {
        m_serial->disconnect();
    } else {
        QString port = m_serialPortCombo->currentText();
        if (!port.isEmpty()) {
            m_serial->connect(port);
        }
    }
}

void MainWindow::onRefreshPortsClicked()
{
    refreshSerialPorts();
    refreshMIDIPorts();
}

void MainWindow::onSerialConnected()
{
    updateConnectionStatus();
    statusBar()->showMessage("Connected to device", 3000);
}

void MainWindow::onSerialDisconnected()
{
    updateConnectionStatus();
    m_boardInfoLabel->hide();
    m_virtualMidiButton->setVisible(true);  // Show again for next connection
    statusBar()->showMessage("Disconnected from device", 3000);
}

void MainWindow::onSerialError(const QString& error)
{
    updateConnectionStatus();
    m_boardInfoLabel->hide();
    statusBar()->showMessage("Connection error: " + error, 5000);
}

void MainWindow::onBoardTypeDetected(BoardType type)
{
    switch (type) {
        case BoardType::Teensy:
            m_boardInfoLabel->setText(
                "<b>Teensy detected</b><br>"
                "Your DAW can connect directly to 'Teensy MIDI' for notes. "
                "This app handles patch editing via serial. "
                "MIDI forwarding disabled to prevent double notes.");
            m_boardInfoLabel->setStyleSheet(
                "color: #8cf; font-size: 11px; padding: 6px; "
                "background-color: #1a3040; border: 1px solid #2a5070; border-radius: 3px;");
            m_midiForwardCheck->setChecked(false);
            m_virtualMidiButton->setVisible(false);  // Not needed for Teensy
            break;

        case BoardType::Arduino:
            m_boardInfoLabel->setText(
                "<b>Arduino detected</b><br>"
                "Enable MIDI forwarding to send notes from your DAW. "
                "Create a virtual MIDI port for your DAW to connect to.");
            m_boardInfoLabel->setStyleSheet(
                "color: #cf8; font-size: 11px; padding: 6px; "
                "background-color: #2a3a1a; border: 1px solid #4a6a2a; border-radius: 3px;");
            m_midiForwardCheck->setChecked(true);
            m_virtualMidiButton->setVisible(true);
            break;

        case BoardType::Unknown:
        default:
            m_boardInfoLabel->setText(
                "<b>Unknown board</b><br>"
                "Could not detect board type. Configure MIDI forwarding manually.");
            m_boardInfoLabel->setStyleSheet(
                "color: #aaa; font-size: 11px; padding: 6px; "
                "background-color: #2a2a2a; border: 1px solid #444; border-radius: 3px;");
            m_virtualMidiButton->setVisible(true);
            break;
    }

    m_boardInfoLabel->show();
}

void MainWindow::onCCReceived(uint8_t channel, uint8_t cc, uint8_t value)
{
    // Flash RX LED to show we received something
    m_midiRxLed->setStyleSheet("background-color: #0f0; color: #000; border: 1px solid #0a0; border-radius: 3px; font-size: 10px;");
    m_midiRxTimer->start();

    // Only update UI if this CC is for the currently selected channel
    uint8_t selectedChannel = m_targetChannel->value() - 1;
    if (channel != selectedChannel) return;

    // Set flag to prevent redundant SysEx sends when updating UI from hardware
    m_updatingFromHardware = true;

    // Block signals to prevent feedback loops
    switch (cc) {
        case 1:  // Mod wheel / LFO
            {
                bool lfoOn = (value > 0);
                m_lfoEnableCheck->blockSignals(true);
                m_lfoEnableCheck->setChecked(lfoOn);
                m_lfoSpeedCombo->setEnabled(lfoOn);
                m_lfoEnableCheck->blockSignals(false);
            }
            break;

        case 10:  // Pan
            {
                int panIndex;
                if (value < 43) panIndex = 0;       // Left
                else if (value > 85) panIndex = 2;  // Right
                else panIndex = 1;                   // Center

                m_panCombo->blockSignals(true);
                m_panCombo->setCurrentIndex(panIndex);
                m_panCombo->blockSignals(false);
            }
            break;

        case 14:  // Algorithm
            if (value < 8) {
                FMPatch patch = m_fmEditor->patch();
                patch.algorithm = value;
                m_fmEditor->setPatch(patch);
            }
            break;

        case 15:  // Feedback
            if (value < 8) {
                FMPatch patch = m_fmEditor->patch();
                patch.feedback = value;
                m_fmEditor->setPatch(patch);
            }
            break;

        case 16: case 17: case 18: case 19:  // Operator TLs
            {
                uint8_t op = cc - 16;
                FMPatch patch = m_fmEditor->patch();
                patch.op[op].tl = value;
                m_fmEditor->setPatch(patch);
            }
            break;

        case 64:  // Sustain pedal (just for display, no UI widget currently)
            // Could add a sustain indicator if desired
            break;
    }

    m_updatingFromHardware = false;
}

void MainWindow::onMIDIPortChanged(int index)
{
    m_midi->closeInputPort();

    if (index > 0) {
        // Index 0 is "(None)"
        if (m_midi->openInputPort(index - 1)) {
            statusBar()->showMessage("MIDI input: " + m_midi->currentInputPort(), 3000);
        }
    }
}

void MainWindow::onMIDIReceived(const std::vector<uint8_t>& message)
{
    // Flash RX LED
    m_midiRxLed->setStyleSheet("background-color: #0f0; color: #000; border: 1px solid #0a0; border-radius: 3px; font-size: 10px;");
    m_midiRxTimer->start();

    if (!m_midiForwardCheck->isChecked()) return;
    if (!m_serial->isConnected()) return;

    // Forward MIDI to serial and flash TX LED
    m_serial->sendRawMIDI(message);
    flashMidiTxLed();
}

void MainWindow::onCreateVirtualPort()
{
    if (m_midi->hasVirtualPort()) {
        m_midi->destroyVirtualInputPort();
        m_virtualMidiButton->setText("Create Virtual Port");
        statusBar()->showMessage("Virtual MIDI port destroyed", 3000);
    } else {
        if (m_midi->createVirtualInputPort("Genesis Engine")) {
            m_virtualMidiButton->setText("Destroy Virtual Port");
            statusBar()->showMessage("Virtual MIDI port created: Genesis Engine", 3000);
            refreshMIDIPorts();
        } else {
            QMessageBox::warning(this, "Error", "Failed to create virtual MIDI port");
        }
    }
}

void MainWindow::onFMPatchSelected(int row)
{
    if (row < 0 || row >= PatchBank::FM_SLOT_COUNT) return;

    m_selectedFMSlot = row;
    m_targetSlot->setValue(row);
    m_fmEditor->setPatch(m_patchBank->fmPatch(row));
}

void MainWindow::onPSGEnvelopeSelected(int row)
{
    if (row < 0 || row >= PatchBank::PSG_SLOT_COUNT) return;

    m_selectedPSGSlot = row;
    m_psgEditor->setEnvelope(m_patchBank->psgEnvelope(row));
}

void MainWindow::onLoadPatchClicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, "Load FM Patch", QString(), FileFormats::loadFilterString());

    if (filePath.isEmpty()) return;

    auto patch = FileFormats::loadFMPatch(filePath);
    if (patch) {
        m_patchBank->setFMPatch(m_selectedFMSlot, *patch);
        updatePatchList();
        m_fmEditor->setPatch(*patch);
        statusBar()->showMessage("Loaded patch: " + patch->name, 3000);
    } else {
        QMessageBox::warning(this, "Error", "Failed to load patch file");
    }
}

void MainWindow::onSavePatchClicked()
{
    QString filePath = QFileDialog::getSaveFileName(
        this, "Save FM Patch", QString(), FileFormats::saveFilterString());

    if (filePath.isEmpty()) return;

    if (!filePath.endsWith(".tfi", Qt::CaseInsensitive)) {
        filePath += ".tfi";
    }

    const FMPatch& patch = m_patchBank->fmPatch(m_selectedFMSlot);
    if (FileFormats::saveTFI(filePath, patch)) {
        statusBar()->showMessage("Saved patch: " + filePath, 3000);
    } else {
        QMessageBox::warning(this, "Error", "Failed to save patch file");
    }
}

void MainWindow::onSendPatchClicked()
{
    if (!m_serial->isConnected()) {
        QMessageBox::warning(this, "Not Connected", "Please connect to a device first.");
        return;
    }

    const FMPatch& patch = m_patchBank->fmPatch(m_selectedFMSlot);
    uint8_t channel = m_targetChannel->value() - 1;  // Convert to 0-indexed
    uint8_t slot = m_targetSlot->value();

    // Send to both slot and channel
    m_serial->sendFMPatchToSlot(slot, patch);
    m_serial->sendFMPatchToChannel(channel, patch);
    flashMidiTxLed();

    statusBar()->showMessage(QString("Sent patch to channel %1 and slot %2")
        .arg(channel + 1).arg(slot), 3000);
}

void MainWindow::onPatchEdited()
{
    FMPatch patch = m_fmEditor->patch();
    patch.name = m_patchBank->fmPatch(m_selectedFMSlot).name;  // Preserve name
    m_patchBank->setFMPatch(m_selectedFMSlot, patch);

    // Live edit: auto-send to device (skip if updating from hardware CC echo)
    if (m_liveEditCheck->isChecked() && !m_updatingFromHardware) {
        sendLivePatch();
    }
}

void MainWindow::onModeChanged(int index)
{
    if (!m_serial->isConnected()) return;

    SynthMode mode = (index == 1) ? SynthMode::Poly : SynthMode::Multi;
    m_serial->setSynthMode(mode);
    statusBar()->showMessage(QString("Synth mode: %1")
        .arg(mode == SynthMode::Poly ? "Poly" : "Multi"), 3000);
}

void MainWindow::onPanChanged(int index)
{
    if (!m_serial->isConnected()) return;

    uint8_t channel = m_targetChannel->value() - 1;

    // Map index to CC 10 value: 0=Left (0), 1=Center (64), 2=Right (127)
    uint8_t panValue;
    switch (index) {
        case 0: panValue = 0; break;    // Left
        case 1: panValue = 64; break;   // Center
        case 2: panValue = 127; break;  // Right
        default: panValue = 64; break;
    }

    m_serial->sendControlChange(channel, 10, panValue);
    flashMidiTxLed();
}

void MainWindow::onLfoEnableChanged(bool enabled)
{
    m_lfoSpeedCombo->setEnabled(enabled);

    if (!m_serial->isConnected()) return;

    uint8_t channel = m_targetChannel->value() - 1;

    // Use CC 1 (Mod Wheel) to enable/disable LFO
    // The firmware enables LFO when CC 1 > 0, disables when CC 1 = 0
    if (enabled) {
        // Send current speed setting as depth
        int speedIndex = m_lfoSpeedCombo->currentIndex();
        uint8_t depth = 64 + speedIndex * 8;  // 64-120 range
        m_serial->sendControlChange(channel, 1, depth);
    } else {
        m_serial->sendControlChange(channel, 1, 0);
    }

    flashMidiTxLed();
}

void MainWindow::onLfoSpeedChanged(int index)
{
    if (!m_serial->isConnected()) return;
    if (!m_lfoEnableCheck->isChecked()) return;

    uint8_t channel = m_targetChannel->value() - 1;

    // When LFO is enabled and speed changes, update the mod wheel value
    // Higher values = more vibrato depth (which triggers LFO in firmware)
    uint8_t depth = 64 + index * 8;
    m_serial->sendControlChange(channel, 1, depth);
    flashMidiTxLed();
}

void MainWindow::onKeyboardNoteOn(int note, int velocity)
{
    if (!m_serial->isConnected()) return;

    // Send Note On on channel 1 (0x90)
    uint8_t channel = m_targetChannel->value() - 1;
    std::vector<uint8_t> msg = {
        static_cast<uint8_t>(0x90 | channel),
        static_cast<uint8_t>(note),
        static_cast<uint8_t>(velocity)
    };
    m_serial->sendRawMIDI(msg);
    flashMidiTxLed();
}

void MainWindow::onKeyboardNoteOff(int note)
{
    if (!m_serial->isConnected()) return;

    // Send Note Off on channel 1 (0x80)
    uint8_t channel = m_targetChannel->value() - 1;
    std::vector<uint8_t> msg = {
        static_cast<uint8_t>(0x80 | channel),
        static_cast<uint8_t>(note),
        static_cast<uint8_t>(0)
    };
    m_serial->sendRawMIDI(msg);
    flashMidiTxLed();
}

void MainWindow::onPanicClicked()
{
    if (!m_serial->isConnected()) {
        statusBar()->showMessage("Not connected - cannot send panic", 3000);
        return;
    }

    // Send All Notes Off (CC 123) and All Sound Off (CC 120) on all 16 channels
    for (uint8_t ch = 0; ch < 16; ch++) {
        // All Sound Off (CC 120) - immediately silences all sound
        m_serial->sendControlChange(ch, 120, 0);
        // All Notes Off (CC 123) - releases all held notes
        m_serial->sendControlChange(ch, 123, 0);
    }

    flashMidiTxLed();
    statusBar()->showMessage("Panic sent - all notes off", 2000);
}

void MainWindow::onRandomizePatchClicked()
{
    auto* rng = QRandomGenerator::global();

    FMPatch patch;
    patch.name = "Random";

    // Algorithm: any of the 8
    patch.algorithm = rng->bounded(8);

    // Feedback: 0-7, but lower values are more common for usable sounds
    patch.feedback = rng->bounded(100) < 70 ? rng->bounded(4) : rng->bounded(8);

    // Determine carrier operators based on algorithm
    // Carriers need different treatment (audible output)
    bool isCarrier[4];
    switch (patch.algorithm) {
        case 0: case 1: case 2: case 3:
            // Algorithms 0-3: only op 4 is carrier
            isCarrier[0] = false; isCarrier[1] = false;
            isCarrier[2] = false; isCarrier[3] = true;
            break;
        case 4:
            // Algorithm 4: ops 2 and 4 are carriers
            isCarrier[0] = false; isCarrier[1] = true;
            isCarrier[2] = false; isCarrier[3] = true;
            break;
        case 5: case 6:
            // Algorithms 5-6: ops 2, 3, 4 are carriers
            isCarrier[0] = false; isCarrier[1] = true;
            isCarrier[2] = true; isCarrier[3] = true;
            break;
        case 7:
            // Algorithm 7: all carriers
            isCarrier[0] = true; isCarrier[1] = true;
            isCarrier[2] = true; isCarrier[3] = true;
            break;
    }

    for (int op = 0; op < 4; op++) {
        FMOperator& o = patch.op[op];

        // MUL: frequency multiplier (0=0.5x, 1-15)
        // Lower values are more common for usable sounds
        if (rng->bounded(100) < 60) {
            o.mul = rng->bounded(1, 5);  // 1-4 most common
        } else {
            o.mul = rng->bounded(16);
        }

        // DT: detune (-3 to +3, stored as 0-7 with 3=center)
        // Usually small detune for slight chorus
        o.dt = rng->bounded(100) < 80 ? rng->bounded(2, 5) : rng->bounded(8);

        // TL: total level (volume, 0=loudest, 127=silent)
        if (isCarrier[op]) {
            // Carriers: moderate volume for audible output
            o.tl = rng->bounded(20, 60);
        } else {
            // Modulators: wider range, can be quiet or loud
            o.tl = rng->bounded(100);
        }

        // RS: rate scaling (0-3) - higher = faster envelopes at high pitches
        o.rs = rng->bounded(100) < 70 ? 0 : rng->bounded(4);

        // AR: attack rate (0-31, higher = faster)
        // Most sounds have fast attack
        o.ar = rng->bounded(100) < 70 ? rng->bounded(20, 31) : rng->bounded(10, 31);

        // DR: decay rate (0-31)
        o.dr = rng->bounded(5, 25);

        // SR: sustain rate (0-31)
        o.sr = rng->bounded(15);

        // RR: release rate (0-15, higher = faster)
        o.rr = rng->bounded(4, 15);

        // SL: sustain level (0-15, 0=loudest sustain, 15=quietest)
        o.sl = rng->bounded(16);

        // SSG-EG: usually off for normal sounds
        o.ssg = rng->bounded(100) < 90 ? 0 : rng->bounded(1, 16);
    }

    // Update the editor and bank
    m_fmEditor->setPatch(patch);
    m_patchBank->setFMPatch(m_selectedFMSlot, patch);
    updatePatchList();

    // If live edit is on, send to device
    if (m_liveEditCheck->isChecked()) {
        sendLivePatch();
    }

    statusBar()->showMessage("Generated random patch", 2000);
}

void MainWindow::onNewBank()
{
    if (m_patchBank->isModified()) {
        int ret = QMessageBox::question(this, "Unsaved Changes",
            "The current bank has unsaved changes. Discard them?",
            QMessageBox::Yes | QMessageBox::No);
        if (ret != QMessageBox::Yes) return;
    }

    m_patchBank->loadDefaults();
    m_currentBankPath.clear();
    updatePatchList();
    statusBar()->showMessage("New bank created", 3000);
}

void MainWindow::onOpenBank()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, "Open Bank", QString(), "Genesis Engine Bank (*.geb);;All Files (*)");

    if (filePath.isEmpty()) return;

    if (m_patchBank->loadBank(filePath)) {
        m_currentBankPath = filePath;
        updatePatchList();
        statusBar()->showMessage("Loaded bank: " + filePath, 3000);
    } else {
        QMessageBox::warning(this, "Error", "Failed to load bank file");
    }
}

void MainWindow::onSaveBank()
{
    if (m_currentBankPath.isEmpty()) {
        onSaveBankAs();
        return;
    }

    if (m_patchBank->saveBank(m_currentBankPath)) {
        m_patchBank->clearModified();
        statusBar()->showMessage("Saved bank: " + m_currentBankPath, 3000);
    } else {
        QMessageBox::warning(this, "Error", "Failed to save bank file");
    }
}

void MainWindow::onSaveBankAs()
{
    QString filePath = QFileDialog::getSaveFileName(
        this, "Save Bank", QString(), "Genesis Engine Bank (*.geb);;All Files (*)");

    if (filePath.isEmpty()) return;

    if (!filePath.endsWith(".geb", Qt::CaseInsensitive)) {
        filePath += ".geb";
    }

    if (m_patchBank->saveBank(filePath)) {
        m_currentBankPath = filePath;
        m_patchBank->clearModified();
        statusBar()->showMessage("Saved bank: " + filePath, 3000);
    } else {
        QMessageBox::warning(this, "Error", "Failed to save bank file");
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_patchBank->isModified()) {
        int ret = QMessageBox::question(this, "Unsaved Changes",
            "The current bank has unsaved changes. Save before closing?",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (ret == QMessageBox::Yes) {
            onSaveBank();
        } else if (ret == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }

    saveSettings();
    event->accept();
}

void MainWindow::loadSettings()
{
    QSettings settings("FM90s", "GenesisEngineSynth");

    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    QString lastPort = settings.value("lastSerialPort").toString();
    if (!lastPort.isEmpty()) {
        int idx = m_serialPortCombo->findText(lastPort, Qt::MatchContains);
        if (idx >= 0) {
            m_serialPortCombo->setCurrentIndex(idx);
        }
    }

    QString lastMidiPort = settings.value("lastMidiPort").toString();
    if (!lastMidiPort.isEmpty()) {
        int idx = m_midiPortCombo->findText(lastMidiPort, Qt::MatchContains);
        if (idx >= 0) {
            m_midiPortCombo->setCurrentIndex(idx);
        }
    }

    // Restore Live Edit state
    m_liveEditCheck->setChecked(settings.value("liveEdit", false).toBool());
}

void MainWindow::saveSettings()
{
    QSettings settings("FM90s", "GenesisEngineSynth");

    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("lastSerialPort", m_serialPortCombo->currentText());
    settings.setValue("lastMidiPort", m_midiPortCombo->currentText());
    settings.setValue("liveEdit", m_liveEditCheck->isChecked());
}

// =============================================================================
// MIDI Activity LEDs
// =============================================================================

void MainWindow::onMidiRxLedTimeout()
{
    m_midiRxLed->setStyleSheet("background-color: #333; color: #666; border: 1px solid #555; border-radius: 3px; font-size: 10px;");
}

void MainWindow::onMidiTxLedTimeout()
{
    m_midiTxLed->setStyleSheet("background-color: #333; color: #666; border: 1px solid #555; border-radius: 3px; font-size: 10px;");
}

void MainWindow::flashMidiTxLed()
{
    m_midiTxLed->setStyleSheet("background-color: #ff0; color: #000; border: 1px solid #aa0; border-radius: 3px; font-size: 10px;");
    m_midiTxTimer->start();
}

// =============================================================================
// Live Edit
// =============================================================================

void MainWindow::sendLivePatch()
{
    if (!m_serial->isConnected()) return;

    const FMPatch& patch = m_patchBank->fmPatch(m_selectedFMSlot);
    uint8_t channel = m_targetChannel->value() - 1;  // Convert to 0-indexed

    // Send to channel only (not slot) for live editing
    m_serial->sendFMPatchToChannel(channel, patch);
    flashMidiTxLed();
}

#include "MainWindow.h"
#include "SerialManager.h"
#include "MIDIManager.h"
#include "PatchBank.h"
#include "FileFormats.h"
#include "FMPatchEditor.h"
#include "PSGEnvelopeEditor.h"

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

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_serial(new SerialManager(this))
    , m_midi(new MIDIManager(this))
    , m_patchBank(new PatchBank(this))
{
    setWindowTitle("Genesis Engine Synth");
    setMinimumSize(1000, 700);

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

    leftLayout->addWidget(fmBankGroup);

    // PSG Envelope Bank
    QGroupBox* psgBankGroup = new QGroupBox("PSG Envelopes");
    QVBoxLayout* psgBankLayout = new QVBoxLayout(psgBankGroup);

    m_psgEnvList = new QListWidget();
    m_psgEnvList->setMaximumHeight(120);
    psgBankLayout->addWidget(m_psgEnvList);

    leftLayout->addWidget(psgBankGroup);
    leftLayout->addStretch();

    // Right panel: Editors
    QTabWidget* editorTabs = new QTabWidget();

    m_fmEditor = new FMPatchEditor();
    editorTabs->addTab(m_fmEditor, "FM Patch Editor");

    m_psgEditor = new PSGEnvelopeEditor();
    editorTabs->addTab(m_psgEditor, "PSG Envelope Editor");

    // Add to splitter
    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(editorTabs);
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

    // Editor connections
    connect(m_fmEditor, &FMPatchEditor::patchChanged, this, &MainWindow::onPatchEdited);

    // Mode
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onModeChanged);
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
    statusBar()->showMessage("Disconnected from device", 3000);
}

void MainWindow::onSerialError(const QString& error)
{
    updateConnectionStatus();
    statusBar()->showMessage("Connection error: " + error, 5000);
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
    if (!m_midiForwardCheck->isChecked()) return;
    if (!m_serial->isConnected()) return;

    // Forward MIDI to serial
    m_serial->sendRawMIDI(message);
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

    statusBar()->showMessage(QString("Sent patch to channel %1 and slot %2")
        .arg(channel + 1).arg(slot), 3000);
}

void MainWindow::onPatchEdited()
{
    FMPatch patch = m_fmEditor->patch();
    patch.name = m_patchBank->fmPatch(m_selectedFMSlot).name;  // Preserve name
    m_patchBank->setFMPatch(m_selectedFMSlot, patch);
}

void MainWindow::onModeChanged(int index)
{
    if (!m_serial->isConnected()) return;

    SynthMode mode = (index == 1) ? SynthMode::Poly : SynthMode::Multi;
    m_serial->setSynthMode(mode);
    statusBar()->showMessage(QString("Synth mode: %1")
        .arg(mode == SynthMode::Poly ? "Poly" : "Multi"), 3000);
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
}

void MainWindow::saveSettings()
{
    QSettings settings("FM90s", "GenesisEngineSynth");

    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("lastSerialPort", m_serialPortCombo->currentText());
    settings.setValue("lastMidiPort", m_midiPortCombo->currentText());
}

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QTimer>
#include "Types.h"

class SerialManager;
class MIDIManager;
class PatchBank;
class FMPatchEditor;
class PSGEnvelopeEditor;
class PianoKeyboardWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // Connection
    void onConnectClicked();
    void onRefreshPortsClicked();
    void onSerialConnected();
    void onSerialDisconnected();
    void onSerialError(const QString& error);
    void onBoardTypeDetected(BoardType type);

    // MIDI
    void onMIDIPortChanged(int index);
    void onMIDIReceived(const std::vector<uint8_t>& message);
    void onCreateVirtualPort();
    void onCCReceived(uint8_t channel, uint8_t cc, uint8_t value);

    // Patch bank
    void onFMPatchSelected(int row);
    void onPSGEnvelopeSelected(int row);
    void onLoadPatchClicked();
    void onSavePatchClicked();
    void onSendPatchClicked();

    // Patch editor
    void onPatchEdited();

    // Mode
    void onModeChanged(int index);

    // Channel controls
    void onPanChanged(int index);
    void onLfoEnableChanged(bool enabled);
    void onLfoSpeedChanged(int index);

    // Keyboard
    void onKeyboardNoteOn(int note, int velocity);
    void onKeyboardNoteOff(int note);
    void onPanicClicked();
    void onRandomizePatchClicked();

    // File menu
    void onNewBank();
    void onOpenBank();
    void onSaveBank();
    void onSaveBankAs();

    // MIDI activity
    void onMidiRxLedTimeout();
    void onMidiTxLedTimeout();

private:
    void setupUI();
    void setupMenus();
    void setupConnections();
    void refreshSerialPorts();
    void refreshMIDIPorts();
    void updateConnectionStatus();
    void updatePatchList();
    void loadSettings();
    void saveSettings();
    void flashMidiTxLed();
    void sendLivePatch();

    // Core managers
    SerialManager* m_serial;
    MIDIManager* m_midi;
    PatchBank* m_patchBank;

    // Connection panel
    QComboBox* m_serialPortCombo;
    QPushButton* m_connectButton;
    QPushButton* m_refreshButton;
    QLabel* m_connectionStatus;
    QLabel* m_boardInfoLabel;

    // MIDI panel
    QComboBox* m_midiPortCombo;
    QPushButton* m_virtualMidiButton;
    QCheckBox* m_midiForwardCheck;

    // Patch bank list
    QListWidget* m_fmPatchList;
    QListWidget* m_psgEnvList;
    QPushButton* m_loadPatchButton;
    QPushButton* m_savePatchButton;
    QPushButton* m_sendPatchButton;
    QPushButton* m_randomizeButton;

    // Editors
    FMPatchEditor* m_fmEditor;
    PSGEnvelopeEditor* m_psgEditor;

    // Keyboard
    PianoKeyboardWidget* m_keyboard;
    QPushButton* m_panicButton;

    // Mode selector
    QComboBox* m_modeCombo;

    // Target selectors
    QSpinBox* m_targetChannel;
    QSpinBox* m_targetSlot;

    // Channel controls (LFO, Pan)
    QComboBox* m_panCombo;
    QCheckBox* m_lfoEnableCheck;
    QComboBox* m_lfoSpeedCombo;

    // Live edit
    QCheckBox* m_liveEditCheck;

    // MIDI activity LEDs
    QLabel* m_midiRxLed;
    QLabel* m_midiTxLed;
    QTimer* m_midiRxTimer;
    QTimer* m_midiTxTimer;

    // State
    QString m_currentBankPath;
    int m_selectedFMSlot = 0;
    int m_selectedPSGSlot = 0;
    bool m_updatingFromHardware = false;  // Prevents redundant SysEx when updating UI from CC echo
};

#endif // MAINWINDOW_H

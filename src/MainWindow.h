#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QCheckBox>
#include "Types.h"

class SerialManager;
class MIDIManager;
class PatchBank;
class FMPatchEditor;
class PSGEnvelopeEditor;

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

    // MIDI
    void onMIDIPortChanged(int index);
    void onMIDIReceived(const std::vector<uint8_t>& message);
    void onCreateVirtualPort();

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

    // File menu
    void onNewBank();
    void onOpenBank();
    void onSaveBank();
    void onSaveBankAs();

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

    // Core managers
    SerialManager* m_serial;
    MIDIManager* m_midi;
    PatchBank* m_patchBank;

    // Connection panel
    QComboBox* m_serialPortCombo;
    QPushButton* m_connectButton;
    QPushButton* m_refreshButton;
    QLabel* m_connectionStatus;

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

    // Editors
    FMPatchEditor* m_fmEditor;
    PSGEnvelopeEditor* m_psgEditor;

    // Mode selector
    QComboBox* m_modeCombo;

    // Target selectors
    QSpinBox* m_targetChannel;
    QSpinBox* m_targetSlot;

    // State
    QString m_currentBankPath;
    int m_selectedFMSlot = 0;
    int m_selectedPSGSlot = 0;
};

#endif // MAINWINDOW_H

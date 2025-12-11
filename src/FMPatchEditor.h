#ifndef FMPATCHEDITOR_H
#define FMPATCHEDITOR_H

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <array>
#include "Types.h"

class OperatorWidget;
class AlgorithmWidget;
class EnvelopeWidget;

/**
 * Complete FM patch editor with 4 operators, algorithm display, and envelope visualization.
 */
class FMPatchEditor : public QWidget
{
    Q_OBJECT

public:
    explicit FMPatchEditor(QWidget* parent = nullptr);

    void setPatch(const FMPatch& patch);
    FMPatch patch() const;

signals:
    void patchChanged();

private slots:
    void onAlgorithmChanged(int value);
    void onFeedbackChanged(int value);
    void onOperatorChanged();
    void onOperatorSelected(int index);

private:
    void setupUI();
    void updateCarrierStates();
    void updateEnvelopeDisplay();

    FMPatch m_patch;
    bool m_updating = false;

    // Global controls
    QComboBox* m_algorithmCombo;
    QSpinBox* m_feedbackSpin;

    // Operator widgets
    std::array<OperatorWidget*, 4> m_operators;

    // Visualization
    AlgorithmWidget* m_algorithmWidget;
    EnvelopeWidget* m_envelopeWidget;
    QComboBox* m_envelopeOpSelect;
};

#endif // FMPATCHEDITOR_H

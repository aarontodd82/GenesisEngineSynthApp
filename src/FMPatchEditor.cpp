#include "FMPatchEditor.h"
#include "OperatorWidget.h"
#include "AlgorithmWidget.h"
#include "EnvelopeWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QScrollArea>

FMPatchEditor::FMPatchEditor(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void FMPatchEditor::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Top section: Algorithm display and envelope previews
    QHBoxLayout* topLayout = new QHBoxLayout();

    // Algorithm visualization
    QGroupBox* algGroup = new QGroupBox("Algorithm");
    QVBoxLayout* algLayout = new QVBoxLayout(algGroup);

    m_algorithmWidget = new AlgorithmWidget();
    algLayout->addWidget(m_algorithmWidget);

    QHBoxLayout* algControls = new QHBoxLayout();
    algControls->addWidget(new QLabel("Algorithm:"));
    m_algorithmCombo = new QComboBox();
    for (int i = 0; i < 8; i++) {
        m_algorithmCombo->addItem(QString::number(i));
    }
    algControls->addWidget(m_algorithmCombo);
    algControls->addWidget(new QLabel("Feedback:"));
    m_feedbackSpin = new QSpinBox();
    m_feedbackSpin->setRange(0, 7);
    algControls->addWidget(m_feedbackSpin);
    algControls->addStretch();
    algLayout->addLayout(algControls);

    topLayout->addWidget(algGroup);

    // Envelope visualizations - 2x2 grid
    QGroupBox* envGroup = new QGroupBox("Operator Envelopes");
    QGridLayout* envGrid = new QGridLayout(envGroup);
    envGrid->setSpacing(4);

    // Visual order: OP1, OP2, OP3, OP4 in 2x2 grid
    // TFI storage: S1(0), S3(1), S2(2), S4(3)
    // Visual OP1=TFI[0], OP2=TFI[2], OP3=TFI[1], OP4=TFI[3]
    int visualToTFI[4] = {0, 2, 1, 3};
    int gridPos[4][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};  // row, col for OP1-4

    for (int vis = 0; vis < 4; vis++) {
        int tfi = visualToTFI[vis];
        m_envelopes[tfi] = new EnvelopeWidget();
        m_envelopes[tfi]->setCompact(true);
        m_envelopes[tfi]->setOperatorNumber(vis + 1);
        envGrid->addWidget(m_envelopes[tfi], gridPos[vis][0], gridPos[vis][1]);

        // Connect envelope drag signals using lambdas to capture the TFI index
        connect(m_envelopes[tfi], &EnvelopeWidget::attackChanged,
                this, [this, tfi](int value) { onEnvelopeAttackChanged(tfi, value); });
        connect(m_envelopes[tfi], &EnvelopeWidget::decayChanged,
                this, [this, tfi](int value) { onEnvelopeDecayChanged(tfi, value); });
        connect(m_envelopes[tfi], &EnvelopeWidget::sustainLevelChanged,
                this, [this, tfi](int value) { onEnvelopeSustainLevelChanged(tfi, value); });
        connect(m_envelopes[tfi], &EnvelopeWidget::releaseChanged,
                this, [this, tfi](int value) { onEnvelopeReleaseChanged(tfi, value); });
    }

    topLayout->addWidget(envGroup, 1);  // Give envelope section more stretch

    mainLayout->addLayout(topLayout);

    // Bottom section: 4 operator editors
    QGroupBox* opsGroup = new QGroupBox("Operators");
    QHBoxLayout* opsLayout = new QHBoxLayout(opsGroup);

    // Create 4 operator widgets in visual order
    for (int vis = 0; vis < 4; vis++) {
        int tfi = visualToTFI[vis];
        m_operators[tfi] = new OperatorWidget(vis);
        opsLayout->addWidget(m_operators[tfi]);

        connect(m_operators[tfi], &OperatorWidget::operatorChanged,
                this, &FMPatchEditor::onOperatorChanged);
    }

    mainLayout->addWidget(opsGroup);

    // Connect signals
    connect(m_algorithmCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FMPatchEditor::onAlgorithmChanged);
    connect(m_feedbackSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &FMPatchEditor::onFeedbackChanged);
    connect(m_algorithmWidget, &AlgorithmWidget::algorithmClicked,
            m_algorithmCombo, &QComboBox::setCurrentIndex);
}

void FMPatchEditor::setPatch(const FMPatch& patch)
{
    m_updating = true;
    m_patch = patch;

    m_algorithmCombo->setCurrentIndex(patch.algorithm);
    m_feedbackSpin->setValue(patch.feedback);
    m_algorithmWidget->setAlgorithm(patch.algorithm);

    for (int i = 0; i < 4; i++) {
        m_operators[i]->setOperator(patch.op[i]);
    }

    updateCarrierStates();
    updateEnvelopeDisplays();

    m_updating = false;
}

FMPatch FMPatchEditor::patch() const
{
    FMPatch p = m_patch;
    p.algorithm = m_algorithmCombo->currentIndex();
    p.feedback = m_feedbackSpin->value();

    for (int i = 0; i < 4; i++) {
        p.op[i] = m_operators[i]->getOperator();
    }

    return p;
}

void FMPatchEditor::onAlgorithmChanged(int value)
{
    m_algorithmWidget->setAlgorithm(value);
    m_patch.algorithm = value;
    updateCarrierStates();

    if (!m_updating) {
        emit patchChanged();
    }
}

void FMPatchEditor::onFeedbackChanged(int value)
{
    m_patch.feedback = value;

    if (!m_updating) {
        emit patchChanged();
    }
}

void FMPatchEditor::onOperatorChanged()
{
    // Update internal patch state
    for (int i = 0; i < 4; i++) {
        m_patch.op[i] = m_operators[i]->getOperator();
    }

    updateEnvelopeDisplays();

    if (!m_updating) {
        emit patchChanged();
    }
}

void FMPatchEditor::onEnvelopeAttackChanged(int opIndex, int value)
{
    if (m_updating) return;

    m_operators[opIndex]->setAR(value);
    m_patch.op[opIndex].ar = value;
    emit patchChanged();
}

void FMPatchEditor::onEnvelopeDecayChanged(int opIndex, int value)
{
    if (m_updating) return;

    m_operators[opIndex]->setDR(value);
    m_patch.op[opIndex].dr = value;
    emit patchChanged();
}

void FMPatchEditor::onEnvelopeSustainLevelChanged(int opIndex, int value)
{
    if (m_updating) return;

    m_operators[opIndex]->setSL(value);
    m_patch.op[opIndex].sl = value;
    emit patchChanged();
}

void FMPatchEditor::onEnvelopeReleaseChanged(int opIndex, int value)
{
    if (m_updating) return;

    m_operators[opIndex]->setRR(value);
    m_patch.op[opIndex].rr = value;
    emit patchChanged();
}

void FMPatchEditor::updateCarrierStates()
{
    bool carriers[4];
    m_algorithmWidget->getCarrierMask(carriers);

    // Visual to TFI mapping
    int visualToTFI[4] = {0, 2, 1, 3};

    for (int vis = 0; vis < 4; vis++) {
        int tfi = visualToTFI[vis];
        m_operators[tfi]->setCarrier(carriers[vis]);
        m_envelopes[tfi]->setIsCarrier(carriers[vis]);
    }
}

void FMPatchEditor::updateEnvelopeDisplays()
{
    for (int i = 0; i < 4; i++) {
        m_envelopes[i]->setOperator(m_patch.op[i]);
    }
}

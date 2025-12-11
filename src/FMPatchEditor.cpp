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

    // Top section: Algorithm display and global controls
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

    // Envelope visualization
    QGroupBox* envGroup = new QGroupBox("Envelope Preview");
    QVBoxLayout* envLayout = new QVBoxLayout(envGroup);

    m_envelopeWidget = new EnvelopeWidget();
    envLayout->addWidget(m_envelopeWidget);

    QHBoxLayout* envControls = new QHBoxLayout();
    envControls->addWidget(new QLabel("Operator:"));
    m_envelopeOpSelect = new QComboBox();
    m_envelopeOpSelect->addItem("OP 1");
    m_envelopeOpSelect->addItem("OP 2");
    m_envelopeOpSelect->addItem("OP 3");
    m_envelopeOpSelect->addItem("OP 4");
    envControls->addWidget(m_envelopeOpSelect);
    envControls->addStretch();
    envLayout->addLayout(envControls);

    topLayout->addWidget(envGroup);

    mainLayout->addLayout(topLayout);

    // Bottom section: 4 operator editors
    QGroupBox* opsGroup = new QGroupBox("Operators");
    QHBoxLayout* opsLayout = new QHBoxLayout(opsGroup);

    // Create 4 operator widgets
    // Visual order: OP1, OP2, OP3, OP4
    // TFI storage order: S1(0), S3(1), S2(2), S4(3)
    // So visual OP1=TFI[0], OP2=TFI[2], OP3=TFI[1], OP4=TFI[3]
    int visualToTFI[4] = {0, 2, 1, 3};

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
    connect(m_envelopeOpSelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FMPatchEditor::onOperatorSelected);
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
    updateEnvelopeDisplay();

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

    updateEnvelopeDisplay();

    if (!m_updating) {
        emit patchChanged();
    }
}

void FMPatchEditor::onOperatorSelected(int index)
{
    // Map visual index to TFI index
    int visualToTFI[4] = {0, 2, 1, 3};
    int tfi = visualToTFI[index];

    const FMOperator& op = m_patch.op[tfi];
    m_envelopeWidget->setEnvelope(op.ar, op.dr, op.sr, op.rr, op.sl, op.tl);
}

void FMPatchEditor::updateCarrierStates()
{
    bool carriers[4];
    m_algorithmWidget->getCarrierMask(carriers);

    for (int i = 0; i < 4; i++) {
        m_operators[i]->setCarrier(carriers[i]);
    }
}

void FMPatchEditor::updateEnvelopeDisplay()
{
    int visIndex = m_envelopeOpSelect->currentIndex();
    int visualToTFI[4] = {0, 2, 1, 3};
    int tfi = visualToTFI[visIndex];

    const FMOperator& op = m_patch.op[tfi];
    m_envelopeWidget->setEnvelope(op.ar, op.dr, op.sr, op.rr, op.sl, op.tl);
}

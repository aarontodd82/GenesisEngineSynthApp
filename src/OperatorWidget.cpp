#include "OperatorWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>

OperatorWidget::OperatorWidget(int operatorNumber, QWidget* parent)
    : QWidget(parent)
    , m_opNumber(operatorNumber)
{
    setupUI();
}

void OperatorWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(2);

    // Title
    m_titleLabel = new QLabel(QString("OP %1").arg(m_opNumber + 1));
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("font-weight: bold; padding: 4px; background: #444; color: white;");
    mainLayout->addWidget(m_titleLabel);

    // Parameters in a grid
    QGridLayout* grid = new QGridLayout();
    grid->setSpacing(2);

    int row = 0;

    // MUL and DT on same row
    grid->addWidget(new QLabel("MUL"), row, 0);
    m_mul = new QSpinBox();
    m_mul->setRange(0, 15);
    m_mul->setToolTip("Multiplier (0=0.5x, 1-15)");
    grid->addWidget(m_mul, row, 1);

    grid->addWidget(new QLabel("DT"), row, 2);
    m_dt = new QSpinBox();
    m_dt->setRange(0, 7);
    m_dt->setToolTip("Detune (0-7, 3=center)");
    grid->addWidget(m_dt, row, 3);
    row++;

    // TL (full width - important param)
    grid->addWidget(new QLabel("TL"), row, 0);
    m_tl = new QSpinBox();
    m_tl->setRange(0, 127);
    m_tl->setToolTip("Total Level / Volume (0=loudest, 127=silent)");
    grid->addWidget(m_tl, row, 1, 1, 3);
    row++;

    // AR and RS
    grid->addWidget(new QLabel("AR"), row, 0);
    m_ar = new QSpinBox();
    m_ar->setRange(0, 31);
    m_ar->setToolTip("Attack Rate (0-31)");
    grid->addWidget(m_ar, row, 1);

    grid->addWidget(new QLabel("RS"), row, 2);
    m_rs = new QSpinBox();
    m_rs->setRange(0, 3);
    m_rs->setToolTip("Rate Scaling (0-3)");
    grid->addWidget(m_rs, row, 3);
    row++;

    // DR and SR
    grid->addWidget(new QLabel("DR"), row, 0);
    m_dr = new QSpinBox();
    m_dr->setRange(0, 31);
    m_dr->setToolTip("Decay Rate (0-31)");
    grid->addWidget(m_dr, row, 1);

    grid->addWidget(new QLabel("SR"), row, 2);
    m_sr = new QSpinBox();
    m_sr->setRange(0, 31);
    m_sr->setToolTip("Sustain Rate (0-31)");
    grid->addWidget(m_sr, row, 3);
    row++;

    // SL and RR
    grid->addWidget(new QLabel("SL"), row, 0);
    m_sl = new QSpinBox();
    m_sl->setRange(0, 15);
    m_sl->setToolTip("Sustain Level (0-15)");
    grid->addWidget(m_sl, row, 1);

    grid->addWidget(new QLabel("RR"), row, 2);
    m_rr = new QSpinBox();
    m_rr->setRange(0, 15);
    m_rr->setToolTip("Release Rate (0-15)");
    grid->addWidget(m_rr, row, 3);
    row++;

    // SSG-EG
    grid->addWidget(new QLabel("SSG"), row, 0);
    m_ssg = new QSpinBox();
    m_ssg->setRange(0, 15);
    m_ssg->setToolTip("SSG-EG mode (0=off, 8-15=enabled)");
    grid->addWidget(m_ssg, row, 1, 1, 3);

    mainLayout->addLayout(grid);
    mainLayout->addStretch();

    // Connect signals
    connect(m_mul, QOverload<int>::of(&QSpinBox::valueChanged), this, &OperatorWidget::onValueChanged);
    connect(m_dt, QOverload<int>::of(&QSpinBox::valueChanged), this, &OperatorWidget::onValueChanged);
    connect(m_tl, QOverload<int>::of(&QSpinBox::valueChanged), this, &OperatorWidget::onValueChanged);
    connect(m_rs, QOverload<int>::of(&QSpinBox::valueChanged), this, &OperatorWidget::onValueChanged);
    connect(m_ar, QOverload<int>::of(&QSpinBox::valueChanged), this, &OperatorWidget::onValueChanged);
    connect(m_dr, QOverload<int>::of(&QSpinBox::valueChanged), this, &OperatorWidget::onValueChanged);
    connect(m_sr, QOverload<int>::of(&QSpinBox::valueChanged), this, &OperatorWidget::onValueChanged);
    connect(m_rr, QOverload<int>::of(&QSpinBox::valueChanged), this, &OperatorWidget::onValueChanged);
    connect(m_sl, QOverload<int>::of(&QSpinBox::valueChanged), this, &OperatorWidget::onValueChanged);
    connect(m_ssg, QOverload<int>::of(&QSpinBox::valueChanged), this, &OperatorWidget::onValueChanged);
}

void OperatorWidget::setOperator(const FMOperator& op)
{
    m_updating = true;

    m_mul->setValue(op.mul);
    m_dt->setValue(op.dt);
    m_tl->setValue(op.tl);
    m_rs->setValue(op.rs);
    m_ar->setValue(op.ar);
    m_dr->setValue(op.dr);
    m_sr->setValue(op.sr);
    m_rr->setValue(op.rr);
    m_sl->setValue(op.sl);
    m_ssg->setValue(op.ssg);

    m_updating = false;
}

FMOperator OperatorWidget::getOperator() const
{
    FMOperator op;
    op.mul = m_mul->value();
    op.dt = m_dt->value();
    op.tl = m_tl->value();
    op.rs = m_rs->value();
    op.ar = m_ar->value();
    op.dr = m_dr->value();
    op.sr = m_sr->value();
    op.rr = m_rr->value();
    op.sl = m_sl->value();
    op.ssg = m_ssg->value();
    return op;
}

void OperatorWidget::setCarrier(bool isCarrier)
{
    m_isCarrier = isCarrier;

    if (isCarrier) {
        m_titleLabel->setStyleSheet("font-weight: bold; padding: 4px; background: #664; color: #ff8;");
        m_titleLabel->setText(QString("OP %1 (C)").arg(m_opNumber + 1));
    } else {
        m_titleLabel->setStyleSheet("font-weight: bold; padding: 4px; background: #446; color: #8cf;");
        m_titleLabel->setText(QString("OP %1 (M)").arg(m_opNumber + 1));
    }
}

void OperatorWidget::onValueChanged()
{
    if (!m_updating) {
        emit operatorChanged();
    }
}

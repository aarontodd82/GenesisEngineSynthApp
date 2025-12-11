#include "PSGEnvelopeEditor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPainter>
#include <QMouseEvent>

// =============================================================================
// PSGEnvelopeEditor
// =============================================================================

PSGEnvelopeEditor::PSGEnvelopeEditor(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PSGEnvelopeEditor::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Envelope display/editor
    QGroupBox* envGroup = new QGroupBox("Volume Envelope (click to edit)");
    QVBoxLayout* envLayout = new QVBoxLayout(envGroup);

    m_envelopeWidget = new PSGEnvelopeWidget();
    envLayout->addWidget(m_envelopeWidget);

    // Controls
    QHBoxLayout* controls = new QHBoxLayout();

    controls->addWidget(new QLabel("Length:"));
    m_lengthSpin = new QSpinBox();
    m_lengthSpin->setRange(1, 64);
    m_lengthSpin->setValue(10);
    controls->addWidget(m_lengthSpin);

    controls->addWidget(new QLabel("Loop Start:"));
    m_loopStartSpin = new QSpinBox();
    m_loopStartSpin->setRange(-1, 63);
    m_loopStartSpin->setSpecialValueText("No Loop");
    m_loopStartSpin->setValue(-1);
    controls->addWidget(m_loopStartSpin);

    controls->addStretch();
    envLayout->addLayout(controls);

    mainLayout->addWidget(envGroup);
    mainLayout->addStretch();

    // Connect signals
    connect(m_lengthSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PSGEnvelopeEditor::onLengthChanged);
    connect(m_loopStartSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PSGEnvelopeEditor::onLoopStartChanged);
    connect(m_envelopeWidget, &PSGEnvelopeWidget::envelopeEdited,
            this, &PSGEnvelopeEditor::onEnvelopeEdited);
}

void PSGEnvelopeEditor::setEnvelope(const PSGEnvelope& env)
{
    m_updating = true;
    m_envelope = env;

    m_lengthSpin->setValue(env.length);
    m_loopStartSpin->setValue(env.loopStart == 0xFF ? -1 : env.loopStart);
    m_envelopeWidget->setEnvelope(env);

    m_updating = false;
}

PSGEnvelope PSGEnvelopeEditor::envelope() const
{
    PSGEnvelope env = m_envelopeWidget->envelope();
    env.length = m_lengthSpin->value();
    env.loopStart = (m_loopStartSpin->value() < 0) ? 0xFF : m_loopStartSpin->value();
    return env;
}

void PSGEnvelopeEditor::onLengthChanged(int value)
{
    m_envelope.length = value;
    m_envelopeWidget->setLength(value);

    // Ensure loop start is valid
    if (m_loopStartSpin->value() >= value) {
        m_loopStartSpin->setValue(-1);
    }
    m_loopStartSpin->setMaximum(value - 1);

    if (!m_updating) {
        emit envelopeChanged();
    }
}

void PSGEnvelopeEditor::onLoopStartChanged(int value)
{
    m_envelope.loopStart = (value < 0) ? 0xFF : value;
    m_envelopeWidget->setLoopStart(m_envelope.loopStart);

    if (!m_updating) {
        emit envelopeChanged();
    }
}

void PSGEnvelopeEditor::onEnvelopeEdited()
{
    m_envelope = m_envelopeWidget->envelope();

    if (!m_updating) {
        emit envelopeChanged();
    }
}

// =============================================================================
// PSGEnvelopeWidget
// =============================================================================

PSGEnvelopeWidget::PSGEnvelopeWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(400, 150);
    setMouseTracking(true);

    // Initialize with a simple decay envelope
    m_envelope.length = 10;
    m_envelope.loopStart = 0xFF;
    for (int i = 0; i < 10; i++) {
        m_envelope.data[i] = i;
    }
}

void PSGEnvelopeWidget::setEnvelope(const PSGEnvelope& env)
{
    m_envelope = env;
    update();
}

void PSGEnvelopeWidget::setLength(uint8_t length)
{
    m_envelope.length = length;
    update();
}

void PSGEnvelopeWidget::setLoopStart(uint8_t loopStart)
{
    m_envelope.loopStart = loopStart;
    update();
}

QSize PSGEnvelopeWidget::sizeHint() const
{
    return QSize(500, 150);
}

QSize PSGEnvelopeWidget::minimumSizeHint() const
{
    return QSize(300, 100);
}

void PSGEnvelopeWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int margin = 20;
    int graphW = w - 2*margin;
    int graphH = h - 2*margin;

    // Background
    painter.fillRect(rect(), QColor(32, 32, 40));

    // Draw grid
    painter.setPen(QPen(QColor(48, 48, 56), 1));

    // Horizontal lines (volume levels)
    for (int i = 0; i <= 15; i++) {
        int y = margin + graphH * i / 15;
        painter.drawLine(margin, y, w - margin, y);
    }

    // Vertical lines (time steps)
    int steps = qMax(1, (int)m_envelope.length);
    float stepW = static_cast<float>(graphW) / steps;
    for (int i = 0; i <= steps; i++) {
        int x = margin + static_cast<int>(i * stepW);
        painter.drawLine(x, margin, x, h - margin);
    }

    // Draw loop marker
    if (m_envelope.loopStart != 0xFF && m_envelope.loopStart < m_envelope.length) {
        int loopX = margin + static_cast<int>(m_envelope.loopStart * stepW);
        painter.setPen(QPen(QColor(100, 200, 100), 2, Qt::DashLine));
        painter.drawLine(loopX, margin, loopX, h - margin);
        painter.setPen(QColor(100, 200, 100));
        painter.drawText(loopX + 2, margin - 2, "Loop");
    }

    // Draw envelope bars
    for (int i = 0; i < m_envelope.length && i < 64; i++) {
        int x = margin + static_cast<int>(i * stepW);
        int barW = static_cast<int>(stepW) - 2;
        if (barW < 2) barW = 2;

        // Volume 0 = loudest (full height), 15 = silent (no bar)
        int vol = m_envelope.data[i] & 0x0F;
        int barH = graphH * (15 - vol) / 15;
        int y = margin + graphH - barH;

        // Color based on volume
        int brightness = 255 - vol * 12;
        painter.fillRect(x + 1, y, barW, barH, QColor(brightness/2, brightness, brightness/2));
    }

    // Draw axis labels
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 8));
    painter.drawText(5, margin + 5, "0");
    painter.drawText(5, h - margin, "15");
    painter.drawText(margin, h - 5, "0");
    painter.drawText(w - margin - 20, h - 5, QString::number(m_envelope.length));
}

void PSGEnvelopeWidget::mousePressEvent(QMouseEvent* event)
{
    int step = stepAtX(event->pos().x());
    int vol = volumeAtY(event->pos().y());

    if (step >= 0 && step < m_envelope.length && vol >= 0 && vol <= 15) {
        m_envelope.data[step] = vol;
        update();
        emit envelopeEdited();
    }
}

void PSGEnvelopeWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        int step = stepAtX(event->pos().x());
        int vol = volumeAtY(event->pos().y());

        if (step >= 0 && step < m_envelope.length && vol >= 0 && vol <= 15) {
            m_envelope.data[step] = vol;
            update();
            emit envelopeEdited();
        }
    }
}

int PSGEnvelopeWidget::stepAtX(int x) const
{
    int margin = 20;
    int graphW = width() - 2*margin;
    int steps = qMax(1, (int)m_envelope.length);

    float stepW = static_cast<float>(graphW) / steps;
    return static_cast<int>((x - margin) / stepW);
}

int PSGEnvelopeWidget::volumeAtY(int y) const
{
    int margin = 20;
    int graphH = height() - 2*margin;

    // Y increases downward, volume 0 is at top (loudest), 15 at bottom (silent)
    int vol = (y - margin) * 15 / graphH;
    return qBound(0, vol, 15);
}

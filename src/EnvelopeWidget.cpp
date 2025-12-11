#include "EnvelopeWidget.h"
#include <QPainter>
#include <QPainterPath>

EnvelopeWidget::EnvelopeWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(200, 80);
}

void EnvelopeWidget::setEnvelope(uint8_t ar, uint8_t dr, uint8_t sr, uint8_t rr, uint8_t sl, uint8_t tl)
{
    m_ar = ar;
    m_dr = dr;
    m_sr = sr;
    m_rr = rr;
    m_sl = sl;
    m_tl = tl;
    update();
}

QSize EnvelopeWidget::sizeHint() const
{
    return QSize(200, 80);
}

QSize EnvelopeWidget::minimumSizeHint() const
{
    return QSize(100, 60);
}

void EnvelopeWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int margin = 10;

    // Background
    painter.fillRect(rect(), QColor(24, 24, 32));

    // Draw grid
    painter.setPen(QPen(QColor(48, 48, 56), 1));
    for (int i = 1; i < 4; i++) {
        int y = margin + (h - 2*margin) * i / 4;
        painter.drawLine(margin, y, w - margin, y);
    }

    // Calculate envelope points
    // Time scaling: higher rate = shorter time
    float attackTime = (31 - m_ar) / 31.0f * 0.25f + 0.02f;   // 2% to 27% of width
    float decayTime = (31 - m_dr) / 31.0f * 0.2f + 0.02f;     // 2% to 22%
    float sustainTime = 0.3f;                                   // Fixed sustain display
    float releaseTime = (15 - m_rr) / 15.0f * 0.2f + 0.02f;   // 2% to 22%

    // Sustain level (0=max volume, 15=min)
    float sustainLevel = m_sl / 15.0f;

    // Total level affects peak
    float peakLevel = 1.0f - (m_tl / 127.0f);

    // Convert to pixel coordinates
    int graphW = w - 2*margin;
    int graphH = h - 2*margin;
    int baseY = h - margin;

    float totalTime = attackTime + decayTime + sustainTime + releaseTime;
    float scale = graphW / totalTime;

    int x0 = margin;
    int x1 = margin + static_cast<int>(attackTime * scale);
    int x2 = x1 + static_cast<int>(decayTime * scale);
    int x3 = x2 + static_cast<int>(sustainTime * scale);
    int x4 = x3 + static_cast<int>(releaseTime * scale);

    int yPeak = baseY - static_cast<int>(graphH * peakLevel);
    int ySustain = baseY - static_cast<int>(graphH * peakLevel * (1.0f - sustainLevel));
    int yZero = baseY;

    // Draw envelope path
    QPainterPath path;
    path.moveTo(x0, yZero);
    path.lineTo(x1, yPeak);        // Attack
    path.lineTo(x2, ySustain);     // Decay to sustain
    path.lineTo(x3, ySustain);     // Sustain hold
    path.lineTo(x4, yZero);        // Release

    // Fill under curve
    QPainterPath fillPath = path;
    fillPath.lineTo(x4, yZero);
    fillPath.lineTo(x0, yZero);
    fillPath.closeSubpath();

    QLinearGradient gradient(0, yPeak, 0, yZero);
    gradient.setColorAt(0, QColor(100, 180, 255, 100));
    gradient.setColorAt(1, QColor(60, 100, 180, 50));
    painter.fillPath(fillPath, gradient);

    // Draw envelope line
    painter.setPen(QPen(QColor(100, 180, 255), 2));
    painter.drawPath(path);

    // Draw phase labels
    painter.setPen(QColor(120, 120, 140));
    painter.setFont(QFont("Arial", 8));
    painter.drawText(x0, h - 2, "A");
    painter.drawText((x1 + x2) / 2 - 4, h - 2, "D");
    painter.drawText((x2 + x3) / 2 - 4, h - 2, "S");
    painter.drawText((x3 + x4) / 2 - 4, h - 2, "R");

    // Draw points
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 200, 100));
    painter.drawEllipse(QPoint(x1, yPeak), 4, 4);
    painter.drawEllipse(QPoint(x2, ySustain), 4, 4);
    painter.drawEllipse(QPoint(x3, ySustain), 4, 4);
}

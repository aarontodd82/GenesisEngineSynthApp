#include "TLBarWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QtMath>

TLBarWidget::TLBarWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
}

QSize TLBarWidget::sizeHint() const
{
    return QSize(20, 100);
}

QSize TLBarWidget::minimumSizeHint() const
{
    return QSize(16, 60);
}

void TLBarWidget::setValue(int value)
{
    if (m_value != value) {
        m_value = qBound(0, value, 127);
        update();
    }
}

int TLBarWidget::valueFromPos(const QPoint& pos) const
{
    int margin = 2;
    int barHeight = height() - 2 * margin;

    // Y position maps inversely to value (top = 0/loudest, bottom = 127/silent)
    float ratio = (float)(pos.y() - margin) / barHeight;
    int value = qBound(0, (int)(ratio * 127), 127);
    return value;
}

void TLBarWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int margin = 2;
    QRect barRect(margin, margin, width() - 2 * margin, height() - 2 * margin);

    // Background
    painter.fillRect(barRect, QColor(30, 30, 35));

    // Border
    painter.setPen(QPen(QColor(60, 60, 70), 1));
    painter.drawRect(barRect);

    // Fill bar (inverted - 0 = full, 127 = empty)
    float fillRatio = 1.0f - (m_value / 127.0f);
    int fillHeight = (int)(barRect.height() * fillRatio);

    QRect fillRect(barRect.left() + 1,
                   barRect.bottom() - fillHeight,
                   barRect.width() - 2,
                   fillHeight);

    // Gradient fill based on carrier/modulator
    QLinearGradient gradient(fillRect.topLeft(), fillRect.bottomLeft());
    if (m_isCarrier) {
        gradient.setColorAt(0, QColor(255, 200, 100));
        gradient.setColorAt(1, QColor(180, 120, 60));
    } else {
        gradient.setColorAt(0, QColor(100, 200, 255));
        gradient.setColorAt(1, QColor(60, 120, 180));
    }
    painter.fillRect(fillRect, gradient);

    // Draw tick marks
    painter.setPen(QPen(QColor(80, 80, 90), 1));
    for (int i = 1; i < 4; i++) {
        int y = barRect.top() + barRect.height() * i / 4;
        painter.drawLine(barRect.left(), y, barRect.left() + 3, y);
        painter.drawLine(barRect.right() - 3, y, barRect.right(), y);
    }

    // Value text at top
    painter.setPen(QColor(200, 200, 210));
    painter.setFont(QFont("Arial", 7));
    QString text = QString::number(m_value);
    QRect textRect(0, 0, width(), 14);
    painter.drawText(textRect, Qt::AlignCenter, text);
}

void TLBarWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        int newValue = valueFromPos(event->pos());
        if (newValue != m_value) {
            m_value = newValue;
            emit valueChanged(m_value);
            update();
        }
    }
}

void TLBarWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging) {
        int newValue = valueFromPos(event->pos());
        if (newValue != m_value) {
            m_value = newValue;
            emit valueChanged(m_value);
            update();
        }
    }
}

void TLBarWidget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    m_dragging = false;
}

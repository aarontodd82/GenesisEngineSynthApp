#include "EnvelopeWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QtMath>

EnvelopeWidget::EnvelopeWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setCursor(Qt::ArrowCursor);
}

void EnvelopeWidget::setOperator(const FMOperator& op)
{
    m_ar = op.ar;
    m_dr = op.dr;
    m_sr = op.sr;
    m_rr = op.rr;
    m_sl = op.sl;
    m_tl = op.tl;
    update();
}

QSize EnvelopeWidget::sizeHint() const
{
    return m_compact ? QSize(120, 80) : QSize(200, 120);
}

QSize EnvelopeWidget::minimumSizeHint() const
{
    return m_compact ? QSize(80, 50) : QSize(120, 80);
}

QRectF EnvelopeWidget::getGraphRect() const
{
    int margin = m_compact ? COMPACT_MARGIN : MARGIN;
    return QRectF(margin, margin, width() - 2 * margin, height() - 2 * margin);
}

QPointF EnvelopeWidget::getAttackPoint() const
{
    QRectF r = getGraphRect();
    // AR: 0 = slowest, 31 = fastest
    // X position: higher AR = shorter attack = point closer to left
    float attackTime = (31 - m_ar) / 31.0f * 0.3f + 0.02f;
    float peakLevel = 1.0f - (m_tl / 127.0f);

    return QPointF(r.left() + r.width() * attackTime,
                   r.bottom() - r.height() * peakLevel);
}

QPointF EnvelopeWidget::getDecayPoint() const
{
    QRectF r = getGraphRect();
    QPointF attack = getAttackPoint();

    float decayTime = (31 - m_dr) / 31.0f * 0.25f + 0.02f;
    float sustainLevel = 1.0f - (m_sl / 15.0f);
    float peakLevel = 1.0f - (m_tl / 127.0f);
    float actualSustain = peakLevel * sustainLevel;

    return QPointF(attack.x() + r.width() * decayTime,
                   r.bottom() - r.height() * actualSustain);
}

QPointF EnvelopeWidget::getSustainPoint() const
{
    QRectF r = getGraphRect();
    QPointF decay = getDecayPoint();

    // Sustain extends for a fixed visual duration
    float sustainTime = 0.25f;

    return QPointF(decay.x() + r.width() * sustainTime, decay.y());
}

QPointF EnvelopeWidget::getReleasePoint() const
{
    QRectF r = getGraphRect();
    QPointF sustain = getSustainPoint();

    float releaseTime = (15 - m_rr) / 15.0f * 0.2f + 0.02f;

    return QPointF(qMin(sustain.x() + r.width() * releaseTime, r.right()),
                   r.bottom());
}

EnvelopeWidget::DragPoint EnvelopeWidget::hitTest(const QPointF& pos) const
{
    const float hitRadius = POINT_RADIUS + 4;

    if (QLineF(pos, getAttackPoint()).length() < hitRadius) return Attack;
    if (QLineF(pos, getDecayPoint()).length() < hitRadius) return Decay;
    if (QLineF(pos, getSustainPoint()).length() < hitRadius) return Sustain;
    if (QLineF(pos, getReleasePoint()).length() < hitRadius) return Release;

    return None;
}

void EnvelopeWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF r = getGraphRect();

    // Background
    QColor bgColor = m_isCarrier ? QColor(40, 35, 30) : QColor(30, 35, 40);
    painter.fillRect(rect(), bgColor);

    // Grid lines
    painter.setPen(QPen(QColor(60, 60, 70), 1));
    for (int i = 1; i < 4; i++) {
        float y = r.top() + r.height() * i / 4;
        painter.drawLine(QPointF(r.left(), y), QPointF(r.right(), y));
    }

    // Get control points
    QPointF startPt(r.left(), r.bottom());
    QPointF attackPt = getAttackPoint();
    QPointF decayPt = getDecayPoint();
    QPointF sustainPt = getSustainPoint();
    QPointF releasePt = getReleasePoint();

    // Draw envelope fill
    QPainterPath fillPath;
    fillPath.moveTo(startPt);
    fillPath.lineTo(attackPt);
    fillPath.lineTo(decayPt);
    fillPath.lineTo(sustainPt);
    fillPath.lineTo(releasePt);
    fillPath.lineTo(r.right(), r.bottom());
    fillPath.lineTo(startPt);
    fillPath.closeSubpath();

    QColor fillColor = m_isCarrier ? QColor(180, 140, 80, 60) : QColor(80, 140, 180, 60);
    painter.fillPath(fillPath, fillColor);

    // Draw envelope line
    QPainterPath linePath;
    linePath.moveTo(startPt);
    linePath.lineTo(attackPt);
    linePath.lineTo(decayPt);
    linePath.lineTo(sustainPt);
    linePath.lineTo(releasePt);

    QColor lineColor = m_isCarrier ? QColor(220, 180, 100) : QColor(100, 180, 220);
    painter.setPen(QPen(lineColor, 2));
    painter.drawPath(linePath);

    // Draw control points
    auto drawPoint = [&](const QPointF& pt, DragPoint type, const QString& label) {
        bool isHovered = (m_hovering == type);
        bool isDragging = (m_dragging == type);

        int radius = POINT_RADIUS;
        if (isHovered || isDragging) radius += 2;

        QColor pointColor = lineColor;
        if (isDragging) pointColor = QColor(255, 220, 100);
        else if (isHovered) pointColor = pointColor.lighter(130);

        painter.setPen(QPen(Qt::white, 2));
        painter.setBrush(pointColor);
        painter.drawEllipse(pt, radius, radius);

        // Label
        if (m_showLabels && !m_compact) {
            painter.setPen(QColor(150, 150, 160));
            painter.setFont(QFont("Arial", 8));
            painter.drawText(QPointF(pt.x() - 5, pt.y() - radius - 4), label);
        }
    };

    if (!m_readOnly) {
        drawPoint(attackPt, Attack, "A");
        drawPoint(decayPt, Decay, "D");
        drawPoint(sustainPt, Sustain, "S");
        drawPoint(releasePt, Release, "R");
    }

    // Operator label
    if (m_opNumber > 0) {
        painter.setPen(m_isCarrier ? QColor(220, 180, 100) : QColor(100, 180, 220));
        painter.setFont(QFont("Arial", m_compact ? 9 : 11, QFont::Bold));
        QString label = QString("OP%1").arg(m_opNumber);
        if (m_isCarrier) label += " (C)";
        else label += " (M)";
        painter.drawText(QPointF(5, m_compact ? 12 : 16), label);
    }

    // Parameter values (bottom)
    if (m_showLabels && !m_compact) {
        painter.setPen(QColor(120, 120, 130));
        painter.setFont(QFont("Arial", 8));
        QString params = QString("AR:%1 DR:%2 SL:%3 RR:%4")
            .arg(m_ar).arg(m_dr).arg(m_sl).arg(m_rr);
        painter.drawText(QPointF(5, height() - 5), params);
    }
}

void EnvelopeWidget::mousePressEvent(QMouseEvent* event)
{
    if (m_readOnly) return;

    m_dragging = hitTest(event->pos());
    if (m_dragging != None) {
        setCursor(Qt::ClosedHandCursor);
        update();
    }
}

void EnvelopeWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_readOnly) return;

    if (m_dragging != None) {
        updateFromDrag(event->pos());
    } else {
        DragPoint hover = hitTest(event->pos());
        if (hover != m_hovering) {
            m_hovering = hover;
            setCursor(hover != None ? Qt::OpenHandCursor : Qt::ArrowCursor);
            update();
        }
    }
}

void EnvelopeWidget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);

    if (m_dragging != None) {
        m_dragging = None;
        setCursor(m_hovering != None ? Qt::OpenHandCursor : Qt::ArrowCursor);
        update();
    }
}

void EnvelopeWidget::updateFromDrag(const QPointF& pos)
{
    QRectF r = getGraphRect();

    // Normalize position to 0-1 range
    float x = qBound(0.0f, (float)(pos.x() - r.left()) / r.width(), 1.0f);
    float y = qBound(0.0f, (float)(pos.y() - r.top()) / r.height(), 1.0f);

    bool changed = false;

    switch (m_dragging) {
        case Attack: {
            // X controls attack rate (left = fast/high AR, right = slow/low AR)
            int newAR = qBound(0, 31 - (int)(x * 31 / 0.32f), 31);
            if (newAR != m_ar) {
                m_ar = newAR;
                emit attackChanged(m_ar);
                changed = true;
            }
            break;
        }
        case Decay: {
            // Y controls sustain level
            float sustainLevel = 1.0f - y;
            int newSL = qBound(0, (int)((1.0f - sustainLevel) * 15), 15);
            if (newSL != m_sl) {
                m_sl = newSL;
                emit sustainLevelChanged(m_sl);
                changed = true;
            }
            // X relative to attack point controls decay rate
            QPointF attackPt = getAttackPoint();
            float decayX = (pos.x() - attackPt.x()) / r.width();
            int newDR = qBound(0, 31 - (int)(decayX * 31 / 0.27f), 31);
            if (newDR != m_dr) {
                m_dr = newDR;
                emit decayChanged(m_dr);
                changed = true;
            }
            break;
        }
        case Sustain: {
            // Y controls sustain level
            float peakLevel = 1.0f - (m_tl / 127.0f);
            float relativeY = y / peakLevel;
            int newSL = qBound(0, (int)(relativeY * 15), 15);
            if (newSL != m_sl) {
                m_sl = newSL;
                emit sustainLevelChanged(m_sl);
                changed = true;
            }
            break;
        }
        case Release: {
            // X relative to sustain point controls release rate
            QPointF sustainPt = getSustainPoint();
            float releaseX = (pos.x() - sustainPt.x()) / r.width();
            int newRR = qBound(0, 15 - (int)(releaseX * 15 / 0.22f), 15);
            if (newRR != m_rr) {
                m_rr = newRR;
                emit releaseChanged(m_rr);
                changed = true;
            }
            break;
        }
        default:
            break;
    }

    if (changed) {
        emit parameterChanged();
        update();
    }
}

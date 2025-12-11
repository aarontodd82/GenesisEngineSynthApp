#include "AlgorithmWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QToolTip>

AlgorithmWidget::AlgorithmWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(220, 140);
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
}

void AlgorithmWidget::setAlgorithm(int algorithm)
{
    if (algorithm >= 0 && algorithm <= 7) {
        m_algorithm = algorithm;
        setToolTip(algorithmDescription(algorithm));
        update();
    }
}

QString AlgorithmWidget::algorithmDescription(int alg) const
{
    switch (alg) {
        case 0: return "Algorithm 0: Serial chain (1->2->3->4)\nBright, cutting sounds";
        case 1: return "Algorithm 1: Parallel mod (1+2)->3->4\nFull, rich sounds";
        case 2: return "Algorithm 2: 1+(2->3)->4\nComplex modulation";
        case 3: return "Algorithm 3: (1->2)+3->4\nBalanced tone";
        case 4: return "Algorithm 4: Dual serial (1->2)+(3->4)\nTwo-voice sounds";
        case 5: return "Algorithm 5: 1->(2+3+4)\nOrgan-like tones";
        case 6: return "Algorithm 6: 1->2 + 3 + 4\nMultiple carriers";
        case 7: return "Algorithm 7: All parallel (1+2+3+4)\nAdditive synthesis";
        default: return "";
    }
}

void AlgorithmWidget::getCarrierMask(bool* isCarrier) const
{
    // TFI operator order: S1(0), S3(1), S2(2), S4(3)
    // Carriers by algorithm:
    isCarrier[0] = isCarrier[1] = isCarrier[2] = isCarrier[3] = false;

    switch (m_algorithm) {
        case 0: case 1: case 2: case 3:
            // S4 only
            isCarrier[3] = true;
            break;
        case 4:
            // S2 and S4
            isCarrier[2] = true;
            isCarrier[3] = true;
            break;
        case 5: case 6:
            // S2, S3, S4
            isCarrier[1] = true;
            isCarrier[2] = true;
            isCarrier[3] = true;
            break;
        case 7:
            // All carriers
            isCarrier[0] = isCarrier[1] = isCarrier[2] = isCarrier[3] = true;
            break;
    }
}

QSize AlgorithmWidget::sizeHint() const
{
    return QSize(240, 160);
}

QSize AlgorithmWidget::minimumSizeHint() const
{
    return QSize(180, 120);
}

void AlgorithmWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Background with hover/press effects
    QColor bgColor(32, 32, 40);
    if (m_pressed) {
        bgColor = bgColor.lighter(80);
    } else if (m_hovered) {
        bgColor = bgColor.lighter(120);
    }
    painter.fillRect(rect(), bgColor);

    // Border when hovered
    if (m_hovered) {
        painter.setPen(QPen(QColor(100, 150, 200), 2));
        painter.drawRect(rect().adjusted(1, 1, -1, -1));
    }

    // Get carrier info
    bool carriers[4];
    getCarrierMask(carriers);

    int w = width();
    int h = height();
    int cx = w / 2;
    int cy = h / 2;

    // Draw algorithm label and hint
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 12, QFont::Bold));
    painter.drawText(8, 20, QString("ALG %1").arg(m_algorithm));

    painter.setPen(QColor(100, 150, 200));
    painter.setFont(QFont("Arial", 9));
    painter.drawText(w - 80, 18, "Click to cycle");

    // Operator positions vary by algorithm
    int opX[4], opY[4];

    switch (m_algorithm) {
        case 0:
            // [1]->[2]->[3]->[4]->OUT (serial)
            opX[0] = cx - 100; opY[0] = cy;
            opX[1] = cx - 35; opY[1] = cy;
            opX[2] = cx + 30; opY[2] = cy;
            opX[3] = cx + 95; opY[3] = cy;
            drawConnection(painter, opX[0]+OP_SIZE/2, opY[0], opX[1]-OP_SIZE/2, opY[1]);
            drawConnection(painter, opX[1]+OP_SIZE/2, opY[1], opX[2]-OP_SIZE/2, opY[2]);
            drawConnection(painter, opX[2]+OP_SIZE/2, opY[2], opX[3]-OP_SIZE/2, opY[3]);
            drawFeedback(painter, opX[0], opY[0]);
            break;

        case 1:
            // [1]+[2]->[3]->[4]->OUT
            opX[0] = cx - 65; opY[0] = cy - 28;
            opX[1] = cx - 65; opY[1] = cy + 28;
            opX[2] = cx + 5; opY[2] = cy;
            opX[3] = cx + 75; opY[3] = cy;
            drawConnection(painter, opX[0]+OP_SIZE/2, opY[0], opX[2]-OP_SIZE/2, opY[2]);
            drawConnection(painter, opX[1]+OP_SIZE/2, opY[1], opX[2]-OP_SIZE/2, opY[2]);
            drawConnection(painter, opX[2]+OP_SIZE/2, opY[2], opX[3]-OP_SIZE/2, opY[3]);
            drawFeedback(painter, opX[0], opY[0]);
            break;

        case 2:
            // [1]+([2]->[3])->[4]->OUT
            opX[0] = cx - 65; opY[0] = cy - 28;
            opX[1] = cx - 65; opY[1] = cy + 28;
            opX[2] = cx + 5; opY[2] = cy + 28;
            opX[3] = cx + 75; opY[3] = cy;
            drawConnection(painter, opX[0]+OP_SIZE/2, opY[0], opX[3]-OP_SIZE/2, opY[3]);
            drawConnection(painter, opX[1]+OP_SIZE/2, opY[1], opX[2]-OP_SIZE/2, opY[2]);
            drawConnection(painter, opX[2]+OP_SIZE/2, opY[2], opX[3]-OP_SIZE/2, opY[3]);
            drawFeedback(painter, opX[0], opY[0]);
            break;

        case 3:
            // ([1]->[2])+[3]->[4]->OUT
            opX[0] = cx - 65; opY[0] = cy - 28;
            opX[1] = cx + 5; opY[1] = cy - 28;
            opX[2] = cx + 5; opY[2] = cy + 28;
            opX[3] = cx + 75; opY[3] = cy;
            drawConnection(painter, opX[0]+OP_SIZE/2, opY[0], opX[1]-OP_SIZE/2, opY[1]);
            drawConnection(painter, opX[1]+OP_SIZE/2, opY[1], opX[3]-OP_SIZE/2, opY[3]);
            drawConnection(painter, opX[2]+OP_SIZE/2, opY[2], opX[3]-OP_SIZE/2, opY[3]);
            drawFeedback(painter, opX[0], opY[0]);
            break;

        case 4:
            // ([1]->[2])+([3]->[4])-> OUT (dual serial)
            opX[0] = cx - 65; opY[0] = cy - 28;
            opX[1] = cx + 5; opY[1] = cy - 28;
            opX[2] = cx - 65; opY[2] = cy + 28;
            opX[3] = cx + 5; opY[3] = cy + 28;
            drawConnection(painter, opX[0]+OP_SIZE/2, opY[0], opX[1]-OP_SIZE/2, opY[1]);
            drawConnection(painter, opX[2]+OP_SIZE/2, opY[2], opX[3]-OP_SIZE/2, opY[3]);
            drawFeedback(painter, opX[0], opY[0]);
            break;

        case 5:
            // [1]->([2]+[3]+[4])-> OUT
            opX[0] = cx - 65; opY[0] = cy;
            opX[1] = cx + 35; opY[1] = cy - 35;
            opX[2] = cx + 35; opY[2] = cy;
            opX[3] = cx + 35; opY[3] = cy + 35;
            drawConnection(painter, opX[0]+OP_SIZE/2, opY[0], opX[1]-OP_SIZE/2, opY[1]);
            drawConnection(painter, opX[0]+OP_SIZE/2, opY[0], opX[2]-OP_SIZE/2, opY[2]);
            drawConnection(painter, opX[0]+OP_SIZE/2, opY[0], opX[3]-OP_SIZE/2, opY[3]);
            drawFeedback(painter, opX[0], opY[0]);
            break;

        case 6:
            // [1]->[2]+[3]+[4]-> OUT
            opX[0] = cx - 65; opY[0] = cy - 35;
            opX[1] = cx + 5; opY[1] = cy - 35;
            opX[2] = cx + 5; opY[2] = cy;
            opX[3] = cx + 5; opY[3] = cy + 35;
            drawConnection(painter, opX[0]+OP_SIZE/2, opY[0], opX[1]-OP_SIZE/2, opY[1]);
            drawFeedback(painter, opX[0], opY[0]);
            break;

        case 7:
            // [1]+[2]+[3]+[4]-> OUT (all parallel)
            opX[0] = cx - 50; opY[0] = cy - 28;
            opX[1] = cx + 50; opY[1] = cy - 28;
            opX[2] = cx - 50; opY[2] = cy + 28;
            opX[3] = cx + 50; opY[3] = cy + 28;
            drawFeedback(painter, opX[0], opY[0]);
            break;
    }

    // Draw operators (TFI order: S1=0, S3=1, S2=2, S4=3)
    for (int vis = 0; vis < 4; vis++) {
        int tfi = (vis == 1) ? 2 : (vis == 2) ? 1 : vis;
        drawOperator(painter, opX[vis], opY[vis], vis + 1, carriers[tfi]);
    }

    // Draw output arrow
    painter.setPen(QPen(QColor(100, 200, 100), 2));
    int outX = w - 25;
    painter.drawLine(outX - 25, cy, outX, cy);
    painter.drawLine(outX - 10, cy - 7, outX, cy);
    painter.drawLine(outX - 10, cy + 7, outX, cy);

    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 9, QFont::Bold));
    painter.drawText(outX - 30, cy + 25, "OUT");
}

void AlgorithmWidget::drawOperator(QPainter& painter, int x, int y, int opNum, bool isCarrier)
{
    QRect rect(x - OP_SIZE/2, y - OP_SIZE/2, OP_SIZE, OP_SIZE);

    // Fill color based on carrier/modulator
    QColor fillColor;
    if (isCarrier) {
        fillColor = QColor(200, 160, 80);
    } else {
        fillColor = QColor(80, 130, 200);
    }

    // Gradient fill for 3D effect
    QLinearGradient gradient(rect.topLeft(), rect.bottomRight());
    gradient.setColorAt(0, fillColor.lighter(120));
    gradient.setColorAt(1, fillColor.darker(110));

    painter.setBrush(gradient);
    painter.setPen(QPen(Qt::white, 2));
    painter.drawRoundedRect(rect, 4, 4);

    // Draw operator number
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 14, QFont::Bold));
    painter.drawText(rect, Qt::AlignCenter, QString::number(opNum));
}

void AlgorithmWidget::drawConnection(QPainter& painter, int x1, int y1, int x2, int y2)
{
    painter.setPen(QPen(QColor(150, 150, 160), 2));
    painter.drawLine(x1, y1, x2, y2);

    // Arrowhead
    double angle = atan2(y2 - y1, x2 - x1);
    int arrowSize = 10;
    QPointF p1(x2 - arrowSize * cos(angle - M_PI/6), y2 - arrowSize * sin(angle - M_PI/6));
    QPointF p2(x2 - arrowSize * cos(angle + M_PI/6), y2 - arrowSize * sin(angle + M_PI/6));
    painter.drawLine(QPointF(x2, y2), p1);
    painter.drawLine(QPointF(x2, y2), p2);
}

void AlgorithmWidget::drawFeedback(QPainter& painter, int x, int y)
{
    // Draw feedback loop arc on operator 1
    painter.setPen(QPen(QColor(220, 100, 100), 2));

    QPainterPath path;
    int r = OP_SIZE/2 + 10;
    path.moveTo(x, y - OP_SIZE/2);
    path.arcTo(x - r, y - OP_SIZE/2 - r, r*2, r*2, 0, 180);

    painter.drawPath(path);

    // Arrowhead on feedback
    painter.drawLine(x - 5, y - OP_SIZE/2 - 5, x, y - OP_SIZE/2);
    painter.drawLine(x + 5, y - OP_SIZE/2 - 5, x, y - OP_SIZE/2);

    // "FB" label
    painter.setFont(QFont("Arial", 8, QFont::Bold));
    painter.drawText(x - 8, y - OP_SIZE/2 - r - 4, "FB");
}

void AlgorithmWidget::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    m_pressed = true;
    update();
}

void AlgorithmWidget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    if (m_pressed) {
        // Cycle through algorithms on click
        int newAlg = (m_algorithm + 1) % 8;
        setAlgorithm(newAlg);
        emit algorithmClicked(newAlg);
    }
    m_pressed = false;
    update();
}

void AlgorithmWidget::enterEvent(QEnterEvent* event)
{
    Q_UNUSED(event);
    m_hovered = true;
    update();
}

void AlgorithmWidget::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    m_hovered = false;
    m_pressed = false;
    update();
}

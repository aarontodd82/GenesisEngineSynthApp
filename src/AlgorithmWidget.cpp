#include "AlgorithmWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>

AlgorithmWidget::AlgorithmWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(200, 120);
}

void AlgorithmWidget::setAlgorithm(int algorithm)
{
    if (algorithm >= 0 && algorithm <= 7) {
        m_algorithm = algorithm;
        update();
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
    return QSize(200, 120);
}

QSize AlgorithmWidget::minimumSizeHint() const
{
    return QSize(150, 100);
}

void AlgorithmWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Background
    painter.fillRect(rect(), QColor(32, 32, 40));

    // Get carrier info
    bool carriers[4];
    getCarrierMask(carriers);

    int w = width();
    int h = height();
    int cx = w / 2;
    int cy = h / 2;

    // Draw algorithm label
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(5, 15, QString("ALG %1").arg(m_algorithm));

    // Operator positions vary by algorithm
    // Using display order: 1, 2, 3, 4 (visual), mapping to TFI: S1, S2, S3, S4
    // But TFI stores: S1(0), S3(1), S2(2), S4(3)

    int opX[4], opY[4];

    switch (m_algorithm) {
        case 0:
            // [1]->[2]->[3]->[4]->OUT (serial)
            opX[0] = cx - 90; opY[0] = cy;
            opX[1] = cx - 30; opY[1] = cy;
            opX[2] = cx + 30; opY[2] = cy;
            opX[3] = cx + 90; opY[3] = cy;
            drawConnection(painter, opX[0]+OP_SIZE/2, opY[0], opX[1]-OP_SIZE/2, opY[1]);
            drawConnection(painter, opX[1]+OP_SIZE/2, opY[1], opX[2]-OP_SIZE/2, opY[2]);
            drawConnection(painter, opX[2]+OP_SIZE/2, opY[2], opX[3]-OP_SIZE/2, opY[3]);
            drawFeedback(painter, opX[0], opY[0]);
            break;

        case 1:
            // [1]+[2]->[3]->[4]->OUT
            opX[0] = cx - 60; opY[0] = cy - 25;
            opX[1] = cx - 60; opY[1] = cy + 25;
            opX[2] = cx; opY[2] = cy;
            opX[3] = cx + 60; opY[3] = cy;
            drawConnection(painter, opX[0]+OP_SIZE/2, opY[0], opX[2]-OP_SIZE/2, opY[2]);
            drawConnection(painter, opX[1]+OP_SIZE/2, opY[1], opX[2]-OP_SIZE/2, opY[2]);
            drawConnection(painter, opX[2]+OP_SIZE/2, opY[2], opX[3]-OP_SIZE/2, opY[3]);
            drawFeedback(painter, opX[0], opY[0]);
            break;

        case 2:
            // [1]+([2]->[3])->[4]->OUT
            opX[0] = cx - 60; opY[0] = cy - 25;
            opX[1] = cx - 60; opY[1] = cy + 25;
            opX[2] = cx; opY[2] = cy + 25;
            opX[3] = cx + 60; opY[3] = cy;
            drawConnection(painter, opX[0]+OP_SIZE/2, opY[0], opX[3]-OP_SIZE/2, opY[3]);
            drawConnection(painter, opX[1]+OP_SIZE/2, opY[1], opX[2]-OP_SIZE/2, opY[2]);
            drawConnection(painter, opX[2]+OP_SIZE/2, opY[2], opX[3]-OP_SIZE/2, opY[3]);
            drawFeedback(painter, opX[0], opY[0]);
            break;

        case 3:
            // ([1]->[2])+[3]->[4]->OUT
            opX[0] = cx - 60; opY[0] = cy - 25;
            opX[1] = cx; opY[1] = cy - 25;
            opX[2] = cx; opY[2] = cy + 25;
            opX[3] = cx + 60; opY[3] = cy;
            drawConnection(painter, opX[0]+OP_SIZE/2, opY[0], opX[1]-OP_SIZE/2, opY[1]);
            drawConnection(painter, opX[1]+OP_SIZE/2, opY[1], opX[3]-OP_SIZE/2, opY[3]);
            drawConnection(painter, opX[2]+OP_SIZE/2, opY[2], opX[3]-OP_SIZE/2, opY[3]);
            drawFeedback(painter, opX[0], opY[0]);
            break;

        case 4:
            // ([1]->[2])+([3]->[4])-> OUT (dual serial)
            opX[0] = cx - 60; opY[0] = cy - 25;
            opX[1] = cx; opY[1] = cy - 25;
            opX[2] = cx - 60; opY[2] = cy + 25;
            opX[3] = cx; opY[3] = cy + 25;
            drawConnection(painter, opX[0]+OP_SIZE/2, opY[0], opX[1]-OP_SIZE/2, opY[1]);
            drawConnection(painter, opX[2]+OP_SIZE/2, opY[2], opX[3]-OP_SIZE/2, opY[3]);
            drawFeedback(painter, opX[0], opY[0]);
            break;

        case 5:
            // [1]->([2]+[3]+[4])-> OUT
            opX[0] = cx - 60; opY[0] = cy;
            opX[1] = cx + 30; opY[1] = cy - 30;
            opX[2] = cx + 30; opY[2] = cy;
            opX[3] = cx + 30; opY[3] = cy + 30;
            drawConnection(painter, opX[0]+OP_SIZE/2, opY[0], opX[1]-OP_SIZE/2, opY[1]);
            drawConnection(painter, opX[0]+OP_SIZE/2, opY[0], opX[2]-OP_SIZE/2, opY[2]);
            drawConnection(painter, opX[0]+OP_SIZE/2, opY[0], opX[3]-OP_SIZE/2, opY[3]);
            drawFeedback(painter, opX[0], opY[0]);
            break;

        case 6:
            // [1]->[2]+[3]+[4]-> OUT
            opX[0] = cx - 60; opY[0] = cy - 30;
            opX[1] = cx; opY[1] = cy - 30;
            opX[2] = cx; opY[2] = cy;
            opX[3] = cx; opY[3] = cy + 30;
            drawConnection(painter, opX[0]+OP_SIZE/2, opY[0], opX[1]-OP_SIZE/2, opY[1]);
            drawFeedback(painter, opX[0], opY[0]);
            break;

        case 7:
            // [1]+[2]+[3]+[4]-> OUT (all parallel)
            opX[0] = cx - 45; opY[0] = cy - 25;
            opX[1] = cx + 45; opY[1] = cy - 25;
            opX[2] = cx - 45; opY[2] = cy + 25;
            opX[3] = cx + 45; opY[3] = cy + 25;
            drawFeedback(painter, opX[0], opY[0]);
            break;
    }

    // Draw operators (TFI order: S1=0, S3=1, S2=2, S4=3)
    // Visual order should be: S1, S2, S3, S4
    // Map visual to TFI: vis[0]=tfi[0], vis[1]=tfi[2], vis[2]=tfi[1], vis[3]=tfi[3]
    int tfiToVis[4] = {0, 2, 1, 3};  // tfi index -> visual position

    for (int vis = 0; vis < 4; vis++) {
        int tfi = (vis == 1) ? 2 : (vis == 2) ? 1 : vis;
        drawOperator(painter, opX[vis], opY[vis], vis + 1, carriers[tfi]);
    }

    // Draw output arrow
    painter.setPen(QPen(QColor(100, 200, 100), 2));
    int outX = w - 30;
    painter.drawLine(outX - 20, cy, outX, cy);
    painter.drawLine(outX - 8, cy - 6, outX, cy);
    painter.drawLine(outX - 8, cy + 6, outX, cy);
    painter.setPen(Qt::white);
    painter.drawText(outX - 25, cy + 25, "OUT");
}

void AlgorithmWidget::drawOperator(QPainter& painter, int x, int y, int opNum, bool isCarrier)
{
    QRect rect(x - OP_SIZE/2, y - OP_SIZE/2, OP_SIZE, OP_SIZE);

    // Fill color based on carrier/modulator
    if (isCarrier) {
        painter.setBrush(QColor(180, 140, 80));
    } else {
        painter.setBrush(QColor(80, 120, 180));
    }

    painter.setPen(QPen(Qt::white, 2));
    painter.drawRect(rect);

    // Draw operator number
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 12, QFont::Bold));
    painter.drawText(rect, Qt::AlignCenter, QString::number(opNum));
}

void AlgorithmWidget::drawConnection(QPainter& painter, int x1, int y1, int x2, int y2)
{
    painter.setPen(QPen(QColor(150, 150, 150), 2));
    painter.drawLine(x1, y1, x2, y2);

    // Arrowhead
    double angle = atan2(y2 - y1, x2 - x1);
    int arrowSize = 8;
    QPointF p1(x2 - arrowSize * cos(angle - M_PI/6), y2 - arrowSize * sin(angle - M_PI/6));
    QPointF p2(x2 - arrowSize * cos(angle + M_PI/6), y2 - arrowSize * sin(angle + M_PI/6));
    painter.drawLine(QPointF(x2, y2), p1);
    painter.drawLine(QPointF(x2, y2), p2);
}

void AlgorithmWidget::drawFeedback(QPainter& painter, int x, int y)
{
    // Draw feedback loop arc on operator 1
    painter.setPen(QPen(QColor(200, 100, 100), 2));

    QPainterPath path;
    int r = OP_SIZE/2 + 8;
    path.moveTo(x, y - OP_SIZE/2);
    path.arcTo(x - r, y - OP_SIZE/2 - r, r*2, r*2, 0, 180);

    painter.drawPath(path);

    // Small "FB" label
    painter.setFont(QFont("Arial", 8));
    painter.drawText(x - 8, y - OP_SIZE/2 - r - 2, "FB");
}

void AlgorithmWidget::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    // Cycle through algorithms on click
    int newAlg = (m_algorithm + 1) % 8;
    setAlgorithm(newAlg);
    emit algorithmClicked(newAlg);
}

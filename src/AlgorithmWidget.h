#ifndef ALGORITHMWIDGET_H
#define ALGORITHMWIDGET_H

#include <QWidget>

/**
 * Visual display of YM2612 FM algorithm routing.
 * Shows how the 4 operators are connected.
 */
class AlgorithmWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AlgorithmWidget(QWidget* parent = nullptr);

    void setAlgorithm(int algorithm);
    int algorithm() const { return m_algorithm; }

    // Returns which operators are carriers for current algorithm
    void getCarrierMask(bool* isCarrier) const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void algorithmClicked(int algorithm);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    void drawOperator(QPainter& painter, int x, int y, int opNum, bool isCarrier);
    void drawConnection(QPainter& painter, int x1, int y1, int x2, int y2);
    void drawFeedback(QPainter& painter, int x, int y);

    int m_algorithm = 0;

    static constexpr int OP_SIZE = 32;
    static constexpr int SPACING = 20;
};

#endif // ALGORITHMWIDGET_H

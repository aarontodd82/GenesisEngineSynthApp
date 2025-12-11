#ifndef TLBARWIDGET_H
#define TLBARWIDGET_H

#include <QWidget>

/**
 * Visual bar display for Total Level (TL) parameter.
 * Shows a vertical bar that represents operator output level.
 * 0 = loudest (full bar), 127 = silent (empty bar).
 */
class TLBarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TLBarWidget(QWidget* parent = nullptr);

    void setValue(int value);
    void setCarrier(bool isCarrier) { m_isCarrier = isCarrier; update(); }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

signals:
    void valueChanged(int value);

private:
    int valueFromPos(const QPoint& pos) const;

    int m_value = 0;
    bool m_isCarrier = false;
    bool m_dragging = false;
};

#endif // TLBARWIDGET_H

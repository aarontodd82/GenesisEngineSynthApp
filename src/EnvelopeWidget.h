#ifndef ENVELOPEWIDGET_H
#define ENVELOPEWIDGET_H

#include <QWidget>
#include "Types.h"

/**
 * Interactive FM operator ADSR envelope editor.
 * Drag the control points to adjust AR, DR, SL, RR.
 */
class EnvelopeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EnvelopeWidget(QWidget* parent = nullptr);

    void setOperator(const FMOperator& op);
    void setReadOnly(bool readOnly) { m_readOnly = readOnly; update(); }
    void setCompact(bool compact) { m_compact = compact; updateGeometry(); update(); }
    void setShowLabels(bool show) { m_showLabels = show; update(); }
    void setOperatorNumber(int num) { m_opNumber = num; update(); }
    void setIsCarrier(bool carrier) { m_isCarrier = carrier; update(); }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void attackChanged(int value);
    void decayChanged(int value);
    void sustainLevelChanged(int value);
    void releaseChanged(int value);
    void parameterChanged();  // Generic signal for any change

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    enum DragPoint { None, Attack, Decay, Sustain, Release };

    QPointF getAttackPoint() const;
    QPointF getDecayPoint() const;
    QPointF getSustainPoint() const;
    QPointF getReleasePoint() const;
    QRectF getGraphRect() const;
    DragPoint hitTest(const QPointF& pos) const;

    void updateFromDrag(const QPointF& pos);

    uint8_t m_ar = 31;
    uint8_t m_dr = 0;
    uint8_t m_sr = 0;
    uint8_t m_rr = 15;
    uint8_t m_sl = 0;
    uint8_t m_tl = 0;

    int m_opNumber = 0;
    bool m_isCarrier = false;
    bool m_readOnly = false;
    bool m_compact = false;
    bool m_showLabels = true;

    DragPoint m_dragging = None;
    DragPoint m_hovering = None;

    static constexpr int POINT_RADIUS = 6;
    static constexpr int MARGIN = 25;
    static constexpr int COMPACT_MARGIN = 15;
};

#endif // ENVELOPEWIDGET_H

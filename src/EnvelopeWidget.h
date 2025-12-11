#ifndef ENVELOPEWIDGET_H
#define ENVELOPEWIDGET_H

#include <QWidget>
#include "Types.h"

/**
 * Visual display of FM operator ADSR envelope.
 */
class EnvelopeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EnvelopeWidget(QWidget* parent = nullptr);

    void setEnvelope(uint8_t ar, uint8_t dr, uint8_t sr, uint8_t rr, uint8_t sl, uint8_t tl);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    uint8_t m_ar = 31;
    uint8_t m_dr = 0;
    uint8_t m_sr = 0;
    uint8_t m_rr = 15;
    uint8_t m_sl = 0;
    uint8_t m_tl = 0;
};

#endif // ENVELOPEWIDGET_H

#ifndef PSGENVELOPEEDITOR_H
#define PSGENVELOPEEDITOR_H

#include <QWidget>
#include <QSpinBox>
#include "Types.h"

class PSGEnvelopeWidget;

/**
 * Editor for PSG software envelopes.
 */
class PSGEnvelopeEditor : public QWidget
{
    Q_OBJECT

public:
    explicit PSGEnvelopeEditor(QWidget* parent = nullptr);

    void setEnvelope(const PSGEnvelope& env);
    PSGEnvelope envelope() const;

signals:
    void envelopeChanged();

private slots:
    void onLengthChanged(int value);
    void onLoopStartChanged(int value);
    void onEnvelopeEdited();

private:
    void setupUI();

    PSGEnvelope m_envelope;
    bool m_updating = false;

    QSpinBox* m_lengthSpin;
    QSpinBox* m_loopStartSpin;
    PSGEnvelopeWidget* m_envelopeWidget;
};

/**
 * Visual editor widget for PSG envelope data.
 */
class PSGEnvelopeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PSGEnvelopeWidget(QWidget* parent = nullptr);

    void setEnvelope(const PSGEnvelope& env);
    PSGEnvelope envelope() const { return m_envelope; }

    void setLength(uint8_t length);
    void setLoopStart(uint8_t loopStart);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void envelopeEdited();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    int stepAtX(int x) const;
    int volumeAtY(int y) const;

    PSGEnvelope m_envelope;
};

#endif // PSGENVELOPEEDITOR_H

#ifndef PIANOKEYBOARDWIDGET_H
#define PIANOKEYBOARDWIDGET_H

#include <QWidget>
#include <QSet>

/**
 * On-screen piano keyboard for testing notes without external MIDI.
 * Supports mouse clicks and computer keyboard input.
 */
class PianoKeyboardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PianoKeyboardWidget(QWidget* parent = nullptr);

    void setBaseOctave(int octave) { m_baseOctave = qBound(0, octave, 8); update(); }
    int baseOctave() const { return m_baseOctave; }

    void setNumOctaves(int octaves) { m_numOctaves = qBound(1, octaves, 4); updateGeometry(); update(); }
    int numOctaves() const { return m_numOctaves; }

    void setVelocity(int velocity) { m_velocity = qBound(1, velocity, 127); }
    int velocity() const { return m_velocity; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void noteOn(int note, int velocity);
    void noteOff(int note);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    int noteFromPos(const QPoint& pos) const;
    QRect keyRect(int note) const;
    bool isBlackKey(int note) const;
    int keyToNote(int key) const;

    int m_baseOctave = 4;
    int m_numOctaves = 2;
    int m_velocity = 100;

    QSet<int> m_pressedNotes;  // Currently pressed notes
    int m_hoveredNote = -1;

    static constexpr int WHITE_KEY_WIDTH = 24;
    static constexpr int WHITE_KEY_HEIGHT = 80;
    static constexpr int BLACK_KEY_WIDTH = 16;
    static constexpr int BLACK_KEY_HEIGHT = 50;
};

#endif // PIANOKEYBOARDWIDGET_H

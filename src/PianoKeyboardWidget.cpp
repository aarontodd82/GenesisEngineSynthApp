#include "PianoKeyboardWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>

PianoKeyboardWidget::PianoKeyboardWidget(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

QSize PianoKeyboardWidget::sizeHint() const
{
    return QSize(WHITE_KEY_WIDTH * 7 * m_numOctaves, WHITE_KEY_HEIGHT);
}

QSize PianoKeyboardWidget::minimumSizeHint() const
{
    return QSize(WHITE_KEY_WIDTH * 7, WHITE_KEY_HEIGHT);
}

bool PianoKeyboardWidget::isBlackKey(int note) const
{
    int n = note % 12;
    return (n == 1 || n == 3 || n == 6 || n == 8 || n == 10);
}

QRect PianoKeyboardWidget::keyRect(int note) const
{
    int octave = note / 12;
    int n = note % 12;
    int baseOctaveOffset = m_baseOctave * 12;
    int relNote = note - baseOctaveOffset;
    int relOctave = relNote / 12;

    // White key positions within octave: C=0, D=1, E=2, F=3, G=4, A=5, B=6
    static const int whiteKeyIndex[] = {0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6};
    // Black key offsets from white key left edge
    static const int blackKeyOffset[] = {-1, 0, -1, 1, -1, -1, 3, -1, 4, -1, 5, -1};

    if (isBlackKey(note)) {
        // Black key
        int whiteIndex = blackKeyOffset[n];
        int x = relOctave * 7 * WHITE_KEY_WIDTH + whiteIndex * WHITE_KEY_WIDTH + WHITE_KEY_WIDTH - BLACK_KEY_WIDTH / 2;
        return QRect(x, 0, BLACK_KEY_WIDTH, BLACK_KEY_HEIGHT);
    } else {
        // White key
        int whiteIndex = whiteKeyIndex[n];
        int x = relOctave * 7 * WHITE_KEY_WIDTH + whiteIndex * WHITE_KEY_WIDTH;
        return QRect(x, 0, WHITE_KEY_WIDTH, WHITE_KEY_HEIGHT);
    }
}

int PianoKeyboardWidget::noteFromPos(const QPoint& pos) const
{
    int baseNote = m_baseOctave * 12;
    int totalWhiteKeys = 7 * m_numOctaves;

    // Check black keys first (they're on top)
    for (int oct = 0; oct < m_numOctaves; oct++) {
        for (int n = 0; n < 12; n++) {
            if (isBlackKey(n)) {
                int note = baseNote + oct * 12 + n;
                QRect r = keyRect(note);
                if (r.contains(pos)) {
                    return note;
                }
            }
        }
    }

    // Then check white keys
    for (int oct = 0; oct < m_numOctaves; oct++) {
        for (int n = 0; n < 12; n++) {
            if (!isBlackKey(n)) {
                int note = baseNote + oct * 12 + n;
                QRect r = keyRect(note);
                if (r.contains(pos)) {
                    return note;
                }
            }
        }
    }

    return -1;
}

int PianoKeyboardWidget::keyToNote(int key) const
{
    // Computer keyboard mapping (QWERTY layout, two rows)
    // Bottom row: Z=C, S=C#, X=D, D=D#, C=E, V=F, G=F#, B=G, H=G#, N=A, J=A#, M=B
    // Top row: Q=C+1oct, 2=C#+1oct, W=D+1oct, etc.

    int baseNote = m_baseOctave * 12;

    switch (key) {
        // Lower octave (bottom row)
        case Qt::Key_Z: return baseNote + 0;   // C
        case Qt::Key_S: return baseNote + 1;   // C#
        case Qt::Key_X: return baseNote + 2;   // D
        case Qt::Key_D: return baseNote + 3;   // D#
        case Qt::Key_C: return baseNote + 4;   // E
        case Qt::Key_V: return baseNote + 5;   // F
        case Qt::Key_G: return baseNote + 6;   // F#
        case Qt::Key_B: return baseNote + 7;   // G
        case Qt::Key_H: return baseNote + 8;   // G#
        case Qt::Key_N: return baseNote + 9;   // A
        case Qt::Key_J: return baseNote + 10;  // A#
        case Qt::Key_M: return baseNote + 11;  // B

        // Upper octave (top row)
        case Qt::Key_Q: return baseNote + 12;  // C
        case Qt::Key_2: return baseNote + 13;  // C#
        case Qt::Key_W: return baseNote + 14;  // D
        case Qt::Key_3: return baseNote + 15;  // D#
        case Qt::Key_E: return baseNote + 16;  // E
        case Qt::Key_R: return baseNote + 17;  // F
        case Qt::Key_5: return baseNote + 18;  // F#
        case Qt::Key_T: return baseNote + 19;  // G
        case Qt::Key_6: return baseNote + 20;  // G#
        case Qt::Key_Y: return baseNote + 21;  // A
        case Qt::Key_7: return baseNote + 22;  // A#
        case Qt::Key_U: return baseNote + 23;  // B
        case Qt::Key_I: return baseNote + 24;  // C (next octave)

        default: return -1;
    }
}

void PianoKeyboardWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int baseNote = m_baseOctave * 12;

    // Draw white keys first
    for (int oct = 0; oct < m_numOctaves; oct++) {
        for (int n = 0; n < 12; n++) {
            if (!isBlackKey(n)) {
                int note = baseNote + oct * 12 + n;
                QRect r = keyRect(note);

                // Fill
                if (m_pressedNotes.contains(note)) {
                    painter.setBrush(QColor(180, 200, 255));
                } else if (note == m_hoveredNote) {
                    painter.setBrush(QColor(240, 240, 245));
                } else {
                    painter.setBrush(Qt::white);
                }

                painter.setPen(QPen(Qt::black, 1));
                painter.drawRect(r);

                // Note name on C keys
                if (n == 0) {
                    painter.setPen(QColor(100, 100, 100));
                    painter.setFont(QFont("Arial", 8));
                    painter.drawText(r.adjusted(2, r.height() - 18, -2, -2), Qt::AlignCenter,
                                     QString("C%1").arg(m_baseOctave + oct));
                }
            }
        }
    }

    // Draw black keys on top
    for (int oct = 0; oct < m_numOctaves; oct++) {
        for (int n = 0; n < 12; n++) {
            if (isBlackKey(n)) {
                int note = baseNote + oct * 12 + n;
                QRect r = keyRect(note);

                // Fill
                if (m_pressedNotes.contains(note)) {
                    painter.setBrush(QColor(100, 120, 180));
                } else if (note == m_hoveredNote) {
                    painter.setBrush(QColor(60, 60, 70));
                } else {
                    painter.setBrush(QColor(30, 30, 35));
                }

                painter.setPen(QPen(Qt::black, 1));
                painter.drawRect(r);
            }
        }
    }

    // Draw keyboard shortcut hints if focused
    if (hasFocus()) {
        painter.setPen(QColor(100, 150, 200));
        painter.setFont(QFont("Arial", 7));
        painter.drawText(5, height() - 5, "Keys: Z-M (low) Q-I (high)");
    }
}

void PianoKeyboardWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        int note = noteFromPos(event->pos());
        if (note >= 0 && !m_pressedNotes.contains(note)) {
            m_pressedNotes.insert(note);
            emit noteOn(note, m_velocity);
            update();
        }
    }
}

void PianoKeyboardWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // Release all mouse-triggered notes
        for (int note : m_pressedNotes) {
            emit noteOff(note);
        }
        m_pressedNotes.clear();
        update();
    }
}

void PianoKeyboardWidget::mouseMoveEvent(QMouseEvent* event)
{
    int note = noteFromPos(event->pos());

    if (note != m_hoveredNote) {
        m_hoveredNote = note;
        update();
    }

    // If dragging, trigger new notes
    if (event->buttons() & Qt::LeftButton) {
        if (note >= 0 && !m_pressedNotes.contains(note)) {
            // Release previous notes and press new one (glide behavior)
            for (int n : m_pressedNotes) {
                emit noteOff(n);
            }
            m_pressedNotes.clear();
            m_pressedNotes.insert(note);
            emit noteOn(note, m_velocity);
            update();
        }
    }
}

void PianoKeyboardWidget::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    m_hoveredNote = -1;
    update();
}

void PianoKeyboardWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat()) return;

    int note = keyToNote(event->key());
    if (note >= 0 && !m_pressedNotes.contains(note)) {
        m_pressedNotes.insert(note);
        emit noteOn(note, m_velocity);
        update();
    }
}

void PianoKeyboardWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat()) return;

    int note = keyToNote(event->key());
    if (note >= 0 && m_pressedNotes.contains(note)) {
        m_pressedNotes.remove(note);
        emit noteOff(note);
        update();
    }
}

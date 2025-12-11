#ifndef OPERATORWIDGET_H
#define OPERATORWIDGET_H

#include <QWidget>
#include <QSpinBox>
#include <QSlider>
#include <QLabel>
#include <QMenu>
#include "Types.h"

class TLBarWidget;

/**
 * Widget for editing a single FM operator's parameters.
 */
class OperatorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OperatorWidget(int operatorNumber, QWidget* parent = nullptr);

    void setOperator(const FMOperator& op);
    FMOperator getOperator() const;

    void setCarrier(bool isCarrier);

    // Individual parameter setters (for envelope drag updates)
    void setAR(int value);
    void setDR(int value);
    void setSL(int value);
    void setRR(int value);

signals:
    void operatorChanged();

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void onValueChanged();
    void onCopyOperator();
    void onPasteOperator();
    void onResetOperator();

private:
    void setupUI();
    QWidget* createParamRow(const QString& label, QSpinBox*& spinBox, int min, int max);

    int m_opNumber;
    bool m_isCarrier = false;
    bool m_updating = false;

    QSpinBox* m_mul;
    QSpinBox* m_dt;
    QSpinBox* m_tl;
    TLBarWidget* m_tlBar;
    QSpinBox* m_rs;
    QSpinBox* m_ar;
    QSpinBox* m_dr;
    QSpinBox* m_sr;
    QSpinBox* m_rr;
    QSpinBox* m_sl;
    QSpinBox* m_ssg;

    QLabel* m_titleLabel;

    // Static clipboard for copy/paste between operators
    static FMOperator s_clipboard;
    static bool s_clipboardValid;
};

#endif // OPERATORWIDGET_H

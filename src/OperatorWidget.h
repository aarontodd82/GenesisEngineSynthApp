#ifndef OPERATORWIDGET_H
#define OPERATORWIDGET_H

#include <QWidget>
#include <QSpinBox>
#include <QSlider>
#include <QLabel>
#include "Types.h"

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

signals:
    void operatorChanged();

private slots:
    void onValueChanged();

private:
    void setupUI();
    QWidget* createParamRow(const QString& label, QSpinBox*& spinBox, int min, int max);

    int m_opNumber;
    bool m_isCarrier = false;
    bool m_updating = false;

    QSpinBox* m_mul;
    QSpinBox* m_dt;
    QSpinBox* m_tl;
    QSpinBox* m_rs;
    QSpinBox* m_ar;
    QSpinBox* m_dr;
    QSpinBox* m_sr;
    QSpinBox* m_rr;
    QSpinBox* m_sl;
    QSpinBox* m_ssg;

    QLabel* m_titleLabel;
};

#endif // OPERATORWIDGET_H

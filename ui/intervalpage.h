#ifndef UI_INTERVALPAGE_H
#define UI_INTERVALPAGE_H

#include "ipage.h"

class QSpinBox;
class QLabel;

class IntervalPage : public IPage
{
    Q_OBJECT
public:
    explicit IntervalPage(QWidget* parent = nullptr);
    QString pageTitle() const override { return QStringLiteral("间隔设置"); }
    Permission requiredPermission() const override { return Permission::IntervalSetting; }
    void onEnter() override;

private slots:
    void onSave();
    void onRestore();

private:
    void setupUi();
    QSpinBox* m_spin = nullptr;
    QLabel* m_hint = nullptr;
};

#endif // UI_INTERVALPAGE_H

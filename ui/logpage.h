#ifndef UI_LOGPAGE_H
#define UI_LOGPAGE_H

#include "ipage.h"

class QTableWidget;

class LogPage : public IPage
{
    Q_OBJECT
public:
    explicit LogPage(QWidget* parent = nullptr);
    QString pageTitle() const override { return QStringLiteral("操作日志"); }
    Permission requiredPermission() const override { return Permission::OperationLog; }
    void onEnter() override;

private:
    void setupUi();
    void refresh();
    QTableWidget* m_table = nullptr;
};

#endif // UI_LOGPAGE_H

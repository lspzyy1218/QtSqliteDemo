#ifndef UI_DRIVERPAGE_H
#define UI_DRIVERPAGE_H

#include "ipage.h"

class QTableWidget;

class DriverPage : public IPage
{
    Q_OBJECT
public:
    explicit DriverPage(QWidget* parent = nullptr);
    QString pageTitle() const override { return QStringLiteral("司机管理"); }
    Permission requiredPermission() const override { return Permission::DriverManage; }
    void onEnter() override;

private slots:
    void onAdd();
    void onEdit();
    void onRemove();

private:
    void setupUi();
    void refresh();
    void openEditor(int id);
    QTableWidget* m_table = nullptr;
};

#endif // UI_DRIVERPAGE_H

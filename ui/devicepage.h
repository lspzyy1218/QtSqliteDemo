#ifndef UI_DEVICEPAGE_H
#define UI_DEVICEPAGE_H

#include "ipage.h"

class QTableWidget;

class DevicePage : public IPage
{
    Q_OBJECT
public:
    explicit DevicePage(QWidget* parent = nullptr);
    QString pageTitle() const override { return QStringLiteral("设备管理"); }
    Permission requiredPermission() const override { return Permission::DeviceManage; }
    void onEnter() override;

private slots:
    void onRemove();
    void onSimulateOnline();

private:
    void setupUi();
    void refresh();
    QTableWidget* m_table = nullptr;
};

#endif // UI_DEVICEPAGE_H

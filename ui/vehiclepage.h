#ifndef UI_VEHICLEPAGE_H
#define UI_VEHICLEPAGE_H

#include "ipage.h"

class QTableWidget;
class QLineEdit;
class QDialog;
class QComboBox;

class VehiclePage : public IPage
{
    Q_OBJECT
public:
    explicit VehiclePage(QWidget* parent = nullptr);
    QString pageTitle() const override { return QStringLiteral("车辆管理"); }
    Permission requiredPermission() const override { return Permission::VehicleManage; }
    void onEnter() override;

private slots:
    void onQuery();
    void onAdd();
    void onEdit();
    void onRemove();
    void onAssociateDriver();

private:
    void setupUi();
    void openEditor(int id);
    QLineEdit* m_search = nullptr;
    QTableWidget* m_table = nullptr;
};

#endif // UI_VEHICLEPAGE_H

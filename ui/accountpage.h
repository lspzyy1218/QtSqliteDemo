#ifndef UI_ACCOUNTPAGE_H
#define UI_ACCOUNTPAGE_H

#include "ipage.h"

class QTableWidget;

class AccountPage : public IPage
{
    Q_OBJECT
public:
    explicit AccountPage(QWidget* parent = nullptr);
    QString pageTitle() const override { return QStringLiteral("账户管理"); }
    Permission requiredPermission() const override { return Permission::AccountManage; }
    void onEnter() override;

private slots:
    void onAdd();
    void onEdit();
    void onRemove();
    void onToggleEnabled();

private:
    void setupUi();
    void refresh();
    void openEditor(int id);
    QTableWidget* m_table = nullptr;
};

#endif // UI_ACCOUNTPAGE_H

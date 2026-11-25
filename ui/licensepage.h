#ifndef UI_LICENSEPAGE_H
#define UI_LICENSEPAGE_H

#include "ipage.h"

class QLabel;
class QLineEdit;
class QDateEdit;

class LicensePage : public IPage
{
    Q_OBJECT
public:
    explicit LicensePage(QWidget* parent = nullptr);
    QString pageTitle() const override { return QStringLiteral("授权管理"); }
    Permission requiredPermission() const override { return Permission::LicenseManage; }
    void onEnter() override;

private slots:
    void onImport();
    void onGenerateFile();
    void onApply();

private:
    void setupUi();
    void refresh();
    QLabel* m_status = nullptr;
    QLineEdit* m_customer = nullptr;
    QLineEdit* m_cipher = nullptr;
    QDateEdit* m_expire = nullptr;
};

#endif // UI_LICENSEPAGE_H

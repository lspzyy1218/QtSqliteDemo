#ifndef CORE_SESSION_H
#define CORE_SESSION_H

#include "models.h"

#include <QObject>

// 全局会话：当前登录账号、权限判断与授权有效期校验
class Session : public QObject
{
    Q_OBJECT
public:
    static Session& instance();

    bool isLoggedIn() const;
    const Account& currentAccount() const;
    Role role() const;

    bool has(Permission p) const;
    bool canEnterDetectionMode() const;

    // 授权校验（批注5）：授权到期日之前才能正常使用系统功能
    bool isLicenseValid() const;
    QString licenseExpireText() const;

    void setAccount(const Account& account);
    void clear();

signals:
    void loginChanged();

private:
    Session() = default;
    Account m_account;
    bool m_loggedIn = false;
};

#endif // CORE_SESSION_H

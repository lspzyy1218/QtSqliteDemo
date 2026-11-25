#include "session.h"
#include "database.h"

#include <QDate>

Session& Session::instance()
{
    static Session s;
    return s;
}

bool Session::isLoggedIn() const
{
    return m_loggedIn;
}

const Account& Session::currentAccount() const
{
    return m_account;
}

Role Session::role() const
{
    return m_account.role;
}

bool Session::has(Permission p) const
{
    if (!m_loggedIn) {
        return false;
    }
    if (m_account.role == Role::Admin) {
        return true; // admin 拥有全部权限
    }
    return m_account.permissions.testFlag(p);
}

bool Session::canEnterDetectionMode() const
{
    return has(Permission::DetectionMode) && isLicenseValid();
}

bool Session::isLicenseValid() const
{
    if (m_account.role == Role::Admin) {
        return true; // admin 全局维护不受授权限制
    }
    const LicenseInfo license = Database::instance().loadLicense();
    if (license.expireDate.isNull()) {
        return false;
    }
    return QDate::currentDate() <= license.expireDate;
}

QString Session::licenseExpireText() const
{
    if (m_loggedIn && m_account.role == Role::Admin) {
        return QStringLiteral("admin 全局维护（不受授权限制）");
    }
    const LicenseInfo license = Database::instance().loadLicense();
    if (license.expireDate.isNull()) {
        return QStringLiteral("未授权");
    }
    return QStringLiteral("授权有效期至 %1").arg(license.expireDate.toString(QStringLiteral("yyyy-MM-dd")));
}

void Session::setAccount(const Account& account)
{
    m_account = account;
    m_loggedIn = true;
    emit loginChanged();
}

void Session::clear()
{
    m_account = Account();
    m_loggedIn = false;
    emit loginChanged();
}

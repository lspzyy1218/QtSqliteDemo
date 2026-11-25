#ifndef CORE_DATABASE_H
#define CORE_DATABASE_H

#include "models.h"

#include <QSqlDatabase>
#include <QVariant>
#include <QVector>
#include <QDate>

class Database
{
public:
    static Database& instance();

    bool open(const QString& dbPath);
    void close();
    bool isOpen() const;
    QString lastError() const;

    // ---- 账号 ----
    bool createAccount(const Account& a);
    bool updateAccount(const Account& a);
    bool deleteAccount(int id);
    Account accountById(int id) const;
    Account accountByUsername(const QString& username) const;
    QVector<Account> allAccounts() const;

    // ---- 司机 ----
    bool createDriver(const Driver& d);
    bool updateDriver(const Driver& d);
    bool deleteDriver(int id);
    QVector<Driver> allDrivers() const;

    // ---- 车辆 ----
    bool createVehicle(const Vehicle& v);
    bool updateVehicle(const Vehicle& v);
    bool deleteVehicle(int id);
    QVector<Vehicle> allVehicles() const;
    QVector<Vehicle> searchVehicles(const QString& plateNo) const;

    // ---- 设备 ----
    bool upsertDevice(const Device& d);
    bool markDeviceRemoved(int id, bool removed);
    bool deleteDevice(int id);
    QVector<Device> allDevices(bool includeRemoved = false) const;
    Device deviceBySerial(const QString& serial) const;

    // ---- 检测记录 ----
    bool createDetection(const Detection& d);
    QVector<Detection> queryDetections(const QDate& from, const QDate& to,
                                       const QString& plateNo, const QString& deviceSerial,
                                       const QString& result) const;
    int todayTotal() const;
    int todayPassed() const;
    int todayFailed() const;
    QVector<int> lastSevenDaysCounts() const; // 最近7天数量，索引0=6天前

    // ---- 授权 ----
    bool saveLicense(const LicenseInfo& info);
    LicenseInfo loadLicense() const;
    bool hasLicense() const;

    // ---- 操作日志 ----
    void log(const QString& user, const QString& action, const QString& detail);
    QVector<OperationLog> queryLogs(int limit = 500) const;

    // ---- 参数设置 ----
    QString setting(const QString& key, const QString& defaultValue = QString()) const;
    void setSetting(const QString& key, const QString& value);
    bool settingRestartRequired(const QString& key) const;
    QVector<QPair<QString, QString>> allSettings() const;

private:
    Database() = default;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool exec(const QString& sql, const QVector<QVariant>& params = {});
    bool createTables();
    void seedDefaults();

    QSqlDatabase m_db;
    mutable QString m_error;
};

#endif // CORE_DATABASE_H

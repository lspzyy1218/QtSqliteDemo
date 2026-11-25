#include "database.h"
#include "crypto.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QtDebug>
#include <QDateTime>

Database& Database::instance()
{
    static Database db;
    return db;
}

bool Database::open(const QString& dbPath)
{
    if (m_db.isOpen()) {
        return true;
    }
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("hmi_main"));
    m_db.setDatabaseName(dbPath);
    if (!m_db.open()) {
        m_error = m_db.lastError().text();
        qWarning() << "[DB] open failed:" << m_error;
        return false;
    }
    return createTables();
}

void Database::close()
{
    const QString name = m_db.connectionName();
    if (m_db.isOpen()) {
        m_db.close();
    }
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(name);
}

bool Database::isOpen() const
{
    return m_db.isOpen();
}

QString Database::lastError() const
{
    return m_error;
}

bool Database::exec(const QString& sql, const QVector<QVariant>& params)
{
    QSqlQuery query(m_db);
    query.prepare(sql);
    for (const QVariant& v : params) {
        query.addBindValue(v);
    }
    if (!query.exec()) {
        m_error = query.lastError().text();
        qWarning() << "[DB] exec failed:" << m_error << sql;
        return false;
    }
    return true;
}

bool Database::createTables()
{
    const QStringList ddl = {
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS accounts ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "username TEXT UNIQUE NOT NULL,"
            "password_hash TEXT NOT NULL,"
            "salt TEXT NOT NULL,"
            "real_name TEXT,"
            "role TEXT NOT NULL DEFAULT 'user',"
            "permissions INTEGER NOT NULL DEFAULT 0,"
            "enabled INTEGER NOT NULL DEFAULT 1,"
            "created_at TEXT)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS drivers ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT NOT NULL,"
            "phone TEXT,"
            "id_card TEXT,"
            "remark TEXT,"
            "created_at TEXT)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS vehicles ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "plate_no TEXT UNIQUE NOT NULL,"
            "energy_type TEXT,"
            "vehicle_type TEXT,"
            "vehicle_level TEXT,"
            "kind TEXT,"
            "load REAL DEFAULT 0,"
            "driver_id INTEGER DEFAULT -1,"
            "remark TEXT,"
            "created_at TEXT)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS devices ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "serial TEXT UNIQUE NOT NULL,"
            "type TEXT,"
            "online INTEGER DEFAULT 0,"
            "last_seen TEXT,"
            "removed INTEGER DEFAULT 0,"
            "remark TEXT)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS detections ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "no TEXT UNIQUE NOT NULL,"
            "device_serial TEXT,"
            "detector TEXT,"
            "detect_time TEXT NOT NULL,"
            "passed INTEGER NOT NULL DEFAULT 1,"
            "plate_no TEXT,"
            "driver_name TEXT,"
            "vehicle_type TEXT,"
            "category TEXT,"
            "load REAL DEFAULT 0,"
            "passengers INTEGER DEFAULT 0,"
            "reason TEXT,"
            "remark TEXT,"
            "temperature REAL DEFAULT 0,"
            "humidity REAL DEFAULT 0,"
            "speed REAL DEFAULT 0,"
            "axle_count INTEGER DEFAULT 0)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS license ("
            "id INTEGER PRIMARY KEY CHECK (id = 1),"
            "customer TEXT,"
            "license_data TEXT,"
            "expire_date TEXT,"
            "issued_date TEXT)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS operation_logs ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "user TEXT,"
            "action TEXT,"
            "detail TEXT,"
            "time TEXT)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS settings ("
            "key TEXT PRIMARY KEY,"
            "value TEXT,"
            "category TEXT DEFAULT 'basic',"
            "restart_required INTEGER DEFAULT 0,"
            "description TEXT)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_detect_time ON detections(detect_time)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_detect_plate ON detections(plate_no)")
    };

    for (const QString& sql : ddl) {
        if (!exec(sql)) {
            return false;
        }
    }
    seedDefaults();
    return true;
}

void Database::seedDefaults()
{
    const QString now = QDateTime::currentDateTime().toString(Qt::ISODate);

    // admin 账号：密码不能默认 123456（批注3）
    if (accountByUsername(QStringLiteral("admin")).id < 0) {
        Account a;
        a.username = QStringLiteral("admin");
        a.realName = QStringLiteral("系统管理员");
        a.role = Role::Admin;
        a.salt = Crypto::makeSalt();
        a.passwordHash = Crypto::passwordHash(QStringLiteral("Admin@2026!Strong"), a.salt);
        a.enabled = true;
        a.createdAt = now;
        createAccount(a);
    }

    // 默认参数（批注6/7/8）
    struct S { const char* key; const char* value; const char* cat; bool restart; const char* desc; };
    const S defaults[] = {
        {"device_detector",      "手持车辆探测仪-01", "basic", false, "手持车辆探测仪名称"},
        {"device_sensor",        "地面传感器-01",     "basic", false, "地面传感器名称"},
        {"detect_mode",          "单车模式",          "detect", false, "默认检测模式"},
        {"detect_timeout",       "30",                "detect", false, "检测超时（秒）"},
        {"detect_new_params",    "true",              "detect", false, "显示新参数（温湿度/速度/轴数）"},
        {"show_chart",           "true",              "detect", false, "显示检测结果图表"},
        {"reinit_interval",      "8",                 "interval", true, "重新初始化检测页面间隔时间（秒）"},
        {"restore_hint",         "恢复默认设置后，所有参数将还原为出厂值，且需重启应用生效", "interval", false, "界面恢复设置说明"},
        {"server_port",          "8080",              "advanced", true, "服务端口（需重启）"},
        {"log_level",            "info",              "advanced", false, "日志级别"}
    };
    for (const S& s : defaults) {
        if (setting(QString::fromLatin1(s.key)).isEmpty()) {
            setSetting(QString::fromLatin1(s.key), QString::fromLatin1(s.value));
            exec(QStringLiteral("UPDATE settings SET category=?, restart_required=?, description=? WHERE key=?"),
                 {QString::fromLatin1(s.cat), s.restart ? 1 : 0,
                  QString::fromLatin1(s.desc), QString::fromLatin1(s.key)});
        }
    }
}

// ==================== 账号 ====================

bool Database::createAccount(const Account& a)
{
    return exec(QStringLiteral(
        "INSERT INTO accounts(username,password_hash,salt,real_name,role,permissions,enabled,created_at) "
        "VALUES(?,?,?,?,?,?,?,?)"),
        {a.username, a.passwordHash, a.salt, a.realName, roleToString(a.role),
         permissionsToInt(a.permissions), a.enabled ? 1 : 0, a.createdAt});
}

bool Database::updateAccount(const Account& a)
{
    return exec(QStringLiteral(
        "UPDATE accounts SET password_hash=?,salt=?,real_name=?,role=?,permissions=?,enabled=? WHERE id=?"),
        {a.passwordHash, a.salt, a.realName, roleToString(a.role),
         permissionsToInt(a.permissions), a.enabled ? 1 : 0, a.id});
}

bool Database::deleteAccount(int id)
{
    return exec(QStringLiteral("DELETE FROM accounts WHERE id=?"), {id});
}

Account Database::accountByUsername(const QString& username) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id,username,password_hash,salt,real_name,role,permissions,enabled,created_at "
                             "FROM accounts WHERE username=?"));
    q.addBindValue(username);
    if (q.exec() && q.next()) {
        Account a;
        a.id = q.value(0).toInt();
        a.username = q.value(1).toString();
        a.passwordHash = q.value(2).toString();
        a.salt = q.value(3).toString();
        a.realName = q.value(4).toString();
        a.role = roleFromString(q.value(5).toString());
        a.permissions = permissionsFromInt(q.value(6).toInt());
        a.enabled = q.value(7).toBool();
        a.createdAt = q.value(8).toString();
        return a;
    }
    return Account();
}

Account Database::accountById(int id) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id,username,password_hash,salt,real_name,role,permissions,enabled,created_at "
                             "FROM accounts WHERE id=?"));
    q.addBindValue(id);
    if (q.exec() && q.next()) {
        Account a;
        a.id = q.value(0).toInt();
        a.username = q.value(1).toString();
        a.passwordHash = q.value(2).toString();
        a.salt = q.value(3).toString();
        a.realName = q.value(4).toString();
        a.role = roleFromString(q.value(5).toString());
        a.permissions = permissionsFromInt(q.value(6).toInt());
        a.enabled = q.value(7).toBool();
        a.createdAt = q.value(8).toString();
        return a;
    }
    return Account();
}

QVector<Account> Database::allAccounts() const
{
    QVector<Account> list;
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("SELECT id,username,real_name,role,permissions,enabled,created_at FROM accounts ORDER BY id"))) {
        while (q.next()) {
            Account a;
            a.id = q.value(0).toInt();
            a.username = q.value(1).toString();
            a.realName = q.value(2).toString();
            a.role = roleFromString(q.value(3).toString());
            a.permissions = permissionsFromInt(q.value(4).toInt());
            a.enabled = q.value(5).toBool();
            a.createdAt = q.value(6).toString();
            list.append(a);
        }
    }
    return list;
}

// ==================== 司机 ====================

bool Database::createDriver(const Driver& d)
{
    return exec(QStringLiteral("INSERT INTO drivers(name,phone,id_card,remark,created_at) VALUES(?,?,?,?,?)"),
                {d.name, d.phone, d.idCard, d.remark, d.createdAt});
}

bool Database::updateDriver(const Driver& d)
{
    return exec(QStringLiteral("UPDATE drivers SET name=?,phone=?,id_card=?,remark=? WHERE id=?"),
                {d.name, d.phone, d.idCard, d.remark, d.id});
}

bool Database::deleteDriver(int id)
{
    exec(QStringLiteral("UPDATE vehicles SET driver_id=-1 WHERE driver_id=?"), {id});
    return exec(QStringLiteral("DELETE FROM drivers WHERE id=?"), {id});
}

QVector<Driver> Database::allDrivers() const
{
    QVector<Driver> list;
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("SELECT id,name,phone,id_card,remark,created_at FROM drivers ORDER BY id"))) {
        while (q.next()) {
            Driver d;
            d.id = q.value(0).toInt();
            d.name = q.value(1).toString();
            d.phone = q.value(2).toString();
            d.idCard = q.value(3).toString();
            d.remark = q.value(4).toString();
            d.createdAt = q.value(5).toString();
            list.append(d);
        }
    }
    return list;
}

// ==================== 车辆 ====================

bool Database::createVehicle(const Vehicle& v)
{
    return exec(QStringLiteral(
        "INSERT INTO vehicles(plate_no,energy_type,vehicle_type,vehicle_level,kind,load,driver_id,remark,created_at) "
        "VALUES(?,?,?,?,?,?,?,?,?)"),
        {v.plateNo, v.energyType, v.vehicleType, v.vehicleLevel, v.kind, v.load, v.driverId, v.remark, v.createdAt});
}

bool Database::updateVehicle(const Vehicle& v)
{
    return exec(QStringLiteral(
        "UPDATE vehicles SET energy_type=?,vehicle_type=?,vehicle_level=?,kind=?,load=?,driver_id=?,remark=? WHERE id=?"),
        {v.energyType, v.vehicleType, v.vehicleLevel, v.kind, v.load, v.driverId, v.remark, v.id});
}

bool Database::deleteVehicle(int id)
{
    return exec(QStringLiteral("DELETE FROM vehicles WHERE id=?"), {id});
}

QVector<Vehicle> Database::allVehicles() const
{
    return searchVehicles(QString());
}

QVector<Vehicle> Database::searchVehicles(const QString& plateNo) const
{
    QVector<Vehicle> list;
    QSqlQuery q(m_db);
    const QString sql = QStringLiteral(
        "SELECT v.id,v.plate_no,v.energy_type,v.vehicle_type,v.vehicle_level,v.kind,v.load,"
        "v.driver_id,COALESCE(d.name,''),v.remark,v.created_at "
        "FROM vehicles v LEFT JOIN drivers d ON v.driver_id=d.id ");
    if (plateNo.trimmed().isEmpty()) {
        q.prepare(sql + QStringLiteral("ORDER BY v.id"));
    } else {
        q.prepare(sql + QStringLiteral("WHERE v.plate_no LIKE ? ORDER BY v.id"));
        q.addBindValue("%" + plateNo.trimmed() + "%");
    }
    if (q.exec()) {
        while (q.next()) {
            Vehicle v;
            v.id = q.value(0).toInt();
            v.plateNo = q.value(1).toString();
            v.energyType = q.value(2).toString();
            v.vehicleType = q.value(3).toString();
            v.vehicleLevel = q.value(4).toString();
            v.kind = q.value(5).toString();
            v.load = q.value(6).toDouble();
            v.driverId = q.value(7).toInt();
            v.driverName = q.value(8).toString();
            v.remark = q.value(9).toString();
            v.createdAt = q.value(10).toString();
            list.append(v);
        }
    }
    return list;
}

// ==================== 设备 ====================

bool Database::upsertDevice(const Device& d)
{
    return exec(QStringLiteral(
        "INSERT INTO devices(serial,type,online,last_seen,removed,remark) VALUES(?,?,?,?,?,?) "
        "ON CONFLICT(serial) DO UPDATE SET type=excluded.type, online=excluded.online, "
        "last_seen=excluded.last_seen, removed=0, remark=excluded.remark"),
        {d.serial, d.type, d.online ? 1 : 0,
         d.lastSeen.isValid() ? d.lastSeen.toString(Qt::ISODate) : QString(), d.removed ? 1 : 0, d.remark});
}

bool Database::markDeviceRemoved(int id, bool removed)
{
    return exec(QStringLiteral("UPDATE devices SET removed=? WHERE id=?"), {removed ? 1 : 0, id});
}

bool Database::deleteDevice(int id)
{
    return exec(QStringLiteral("DELETE FROM devices WHERE id=?"), {id});
}

Device Database::deviceBySerial(const QString& serial) const
{
    Device d;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id,serial,type,online,last_seen,removed,remark FROM devices WHERE serial=?"));
    q.addBindValue(serial);
    if (q.exec() && q.next()) {
        d.id = q.value(0).toInt();
        d.serial = q.value(1).toString();
        d.type = q.value(2).toString();
        d.online = q.value(3).toBool();
        d.lastSeen = QDateTime::fromString(q.value(4).toString(), Qt::ISODate);
        d.removed = q.value(5).toBool();
        d.remark = q.value(6).toString();
    }
    return d;
}

QVector<Device> Database::allDevices(bool includeRemoved) const
{
    QVector<Device> list;
    QSqlQuery q(m_db);
    const QString sql = QStringLiteral("SELECT id,serial,type,online,last_seen,removed,remark FROM devices ") +
                        (includeRemoved ? QString() : QStringLiteral("WHERE removed=0 ")) +
                        QStringLiteral("ORDER BY id");
    if (q.exec(sql)) {
        while (q.next()) {
            Device d;
            d.id = q.value(0).toInt();
            d.serial = q.value(1).toString();
            d.type = q.value(2).toString();
            d.online = q.value(3).toBool();
            d.lastSeen = QDateTime::fromString(q.value(4).toString(), Qt::ISODate);
            d.removed = q.value(5).toBool();
            d.remark = q.value(6).toString();
            list.append(d);
        }
    }
    return list;
}

// ==================== 检测记录 ====================

bool Database::createDetection(const Detection& d)
{
    return exec(QStringLiteral(
        "INSERT INTO detections(no,device_serial,detector,detect_time,passed,plate_no,driver_name,"
        "vehicle_type,category,load,passengers,reason,remark,temperature,humidity,speed,axle_count) "
        "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"),
        {d.no, d.deviceSerial, d.detector, d.time.toString(Qt::ISODate), d.passed ? 1 : 0,
         d.plateNo, d.driverName, d.vehicleType, d.category, d.load, d.passengers,
         d.reason, d.remark, d.temperature, d.humidity, d.speed, d.axleCount});
}

QVector<Detection> Database::queryDetections(const QDate& from, const QDate& to,
                                             const QString& plateNo, const QString& deviceSerial,
                                             const QString& result) const
{
    QVector<Detection> list;
    QSqlQuery q(m_db);
    QString sql = QStringLiteral(
        "SELECT id,no,device_serial,detector,detect_time,passed,plate_no,driver_name,vehicle_type,"
        "category,load,passengers,reason,remark,temperature,humidity,speed,axle_count "
        "FROM detections WHERE 1=1 ");
    QVector<QVariant> params;

    // 批注2-3：修复日期查询缺失，使用 date() 明确按日期比较，且结束日期包含当天
    if (from.isValid()) { sql += QStringLiteral("AND date(detect_time)>=date(?) "); params.append(from.toString(Qt::ISODate)); }
    if (to.isValid())   { sql += QStringLiteral("AND date(detect_time)<=date(?) "); params.append(to.toString(Qt::ISODate)); }
    if (!plateNo.trimmed().isEmpty()) { sql += QStringLiteral("AND plate_no LIKE ? "); params.append("%" + plateNo.trimmed() + "%"); }
    if (!deviceSerial.trimmed().isEmpty()) { sql += QStringLiteral("AND device_serial LIKE ? "); params.append("%" + deviceSerial.trimmed() + "%"); }
    if (result == QStringLiteral("通过"))   { sql += QStringLiteral("AND passed=1 "); }
    if (result == QStringLiteral("不通过")) { sql += QStringLiteral("AND passed=0 "); }
    sql += QStringLiteral("ORDER BY detect_time DESC");

    q.prepare(sql);
    for (const QVariant& p : params) q.addBindValue(p);
    if (q.exec()) {
        while (q.next()) {
            Detection d;
            d.id = q.value(0).toInt();
            d.no = q.value(1).toString();
            d.deviceSerial = q.value(2).toString();
            d.detector = q.value(3).toString();
            d.time = QDateTime::fromString(q.value(4).toString(), Qt::ISODate);
            d.passed = q.value(5).toBool();
            d.plateNo = q.value(6).toString();
            d.driverName = q.value(7).toString();
            d.vehicleType = q.value(8).toString();
            d.category = q.value(9).toString();
            d.load = q.value(10).toDouble();
            d.passengers = q.value(11).toInt();
            d.reason = q.value(12).toString();
            d.remark = q.value(13).toString();
            d.temperature = q.value(14).toDouble();
            d.humidity = q.value(15).toDouble();
            d.speed = q.value(16).toDouble();
            d.axleCount = q.value(17).toInt();
            list.append(d);
        }
    }
    return list;
}

int Database::todayTotal() const
{
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("SELECT COUNT(*) FROM detections WHERE date(detect_time)=date('now','localtime')")) && q.next())
        return q.value(0).toInt();
    return 0;
}

int Database::todayPassed() const
{
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("SELECT COUNT(*) FROM detections WHERE date(detect_time)=date('now','localtime') AND passed=1")) && q.next())
        return q.value(0).toInt();
    return 0;
}

int Database::todayFailed() const
{
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("SELECT COUNT(*) FROM detections WHERE date(detect_time)=date('now','localtime') AND passed=0")) && q.next())
        return q.value(0).toInt();
    return 0;
}

QVector<int> Database::lastSevenDaysCounts() const
{
    QVector<int> result(7, 0);
    QSqlQuery q(m_db);
    const QString sql = QStringLiteral(
        "SELECT CAST(strftime('%%d', 'now','localtime', '-' || (6-n) || ' days') AS INTEGER) AS d, COUNT(*) "
        "FROM detections WHERE date(detect_time) >= date('now','localtime','-6 days') GROUP BY d");
    // SQLite 递归生成日期较复杂，改用逐日查询保证正确性
    for (int i = 6; i >= 0; --i) {
        QSqlQuery day(m_db);
        day.prepare(QStringLiteral(
            "SELECT COUNT(*) FROM detections WHERE date(detect_time)=date('now','localtime',?)"));
        day.addBindValue(QString::number(-i) + QStringLiteral(" days"));
        if (day.exec() && day.next()) {
            result[6 - i] = day.value(0).toInt();
        }
    }
    Q_UNUSED(q)
    Q_UNUSED(sql)
    return result;
}

// ==================== 授权 ====================

bool Database::saveLicense(const LicenseInfo& info)
{
    return exec(QStringLiteral("INSERT INTO license(id,customer,license_data,expire_date,issued_date) "
                               "VALUES(1,?,?,?,?) ON CONFLICT(id) DO UPDATE SET "
                               "customer=excluded.customer,license_data=excluded.license_data,"
                               "expire_date=excluded.expire_date,issued_date=excluded.issued_date"),
                {info.customer, info.licenseData, info.expireDate.toString(Qt::ISODate),
                 info.issuedDate.toString(Qt::ISODate)});
}

LicenseInfo Database::loadLicense() const
{
    LicenseInfo info;
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("SELECT customer,license_data,expire_date,issued_date FROM license WHERE id=1")) && q.next()) {
        info.customer = q.value(0).toString();
        info.licenseData = q.value(1).toString();
        info.expireDate = QDate::fromString(q.value(2).toString(), Qt::ISODate);
        info.issuedDate = QDate::fromString(q.value(3).toString(), Qt::ISODate);
        info.plainSummary = Crypto::decrypt(info.licenseData, Crypto::licenseKey());
    }
    return info;
}

bool Database::hasLicense() const
{
    QSqlQuery q(m_db);
    return q.exec(QStringLiteral("SELECT COUNT(*) FROM license WHERE id=1")) && q.next() && q.value(0).toInt() > 0;
}

// ==================== 操作日志 ====================

void Database::log(const QString& user, const QString& action, const QString& detail)
{
    exec(QStringLiteral("INSERT INTO operation_logs(user,action,detail,time) VALUES(?,?,?,?)"),
         {user, action, detail, QDateTime::currentDateTime().toString(Qt::ISODate)});
}

QVector<OperationLog> Database::queryLogs(int limit) const
{
    QVector<OperationLog> list;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id,user,action,detail,time FROM operation_logs ORDER BY id DESC LIMIT ?"));
    q.addBindValue(limit);
    if (q.exec()) {
        while (q.next()) {
            OperationLog l;
            l.id = q.value(0).toInt();
            l.user = q.value(1).toString();
            l.action = q.value(2).toString();
            l.detail = q.value(3).toString();
            l.time = QDateTime::fromString(q.value(4).toString(), Qt::ISODate);
            list.append(l);
        }
    }
    return list;
}

// ==================== 参数设置 ====================

QString Database::setting(const QString& key, const QString& defaultValue) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT value FROM settings WHERE key=?"));
    q.addBindValue(key);
    if (q.exec() && q.next()) {
        return q.value(0).toString();
    }
    return defaultValue;
}

void Database::setSetting(const QString& key, const QString& value)
{
    exec(QStringLiteral("INSERT INTO settings(key,value) VALUES(?,?) "
                        "ON CONFLICT(key) DO UPDATE SET value=excluded.value"), {key, value});
}

bool Database::settingRestartRequired(const QString& key) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT restart_required FROM settings WHERE key=?"));
    q.addBindValue(key);
    if (q.exec() && q.next()) {
        return q.value(0).toBool();
    }
    return false;
}

QVector<QPair<QString, QString>> Database::allSettings() const
{
    QVector<QPair<QString, QString>> list;
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("SELECT key,value FROM settings ORDER BY key"))) {
        while (q.next()) {
            list.append(qMakePair(q.value(0).toString(), q.value(1).toString()));
        }
    }
    return list;
}

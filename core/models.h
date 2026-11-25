#ifndef CORE_MODELS_H
#define CORE_MODELS_H

#include <QString>
#include <QDateTime>

// 账号角色：admin（全局维护，不交付客户）/ manager（管理员）/ user（普通用户）
enum class Role { Admin, Manager, User };

inline QString roleToString(Role role)
{
    switch (role) {
    case Role::Admin:   return QStringLiteral("admin");
    case Role::Manager: return QStringLiteral("管理员");
    case Role::User:    return QStringLiteral("普通用户");
    }
    return QString();
}

inline Role roleFromString(const QString& s)
{
    if (s == QStringLiteral("admin") || s == QLatin1String("admin")) return Role::Admin;
    if (s == QStringLiteral("管理员")) return Role::Manager;
    return Role::User;
}

// 权限项（对应系统菜单）
enum class Permission {
    DetectionRecord = 1 << 0,
    DeviceManage    = 1 << 1,
    AccountManage   = 1 << 2,
    DriverManage    = 1 << 3,
    VehicleManage   = 1 << 4,
    LicenseManage   = 1 << 5,
    OperationLog    = 1 << 6,
    Settings        = 1 << 7,
    IntervalSetting = 1 << 8,
    DetectionMode   = 1 << 9,
};
Q_DECLARE_FLAGS(Permissions, Permission)
Q_DECLARE_OPERATORS_FOR_FLAGS(Permissions)

inline Permissions permissionsFromInt(int value)
{
    return Permissions(QFlags<Permission>::Int(value));
}

inline int permissionsToInt(Permissions permissions)
{
    return static_cast<int>(permissions.toInt());
}

struct Account {
    int id = -1;
    QString username;
    QString passwordHash;
    QString salt;
    QString realName;
    Role role = Role::User;
    Permissions permissions;
    bool enabled = true;
    QString createdAt;
};

struct Driver {
    int id = -1;
    QString name;
    QString phone;
    QString idCard;
    QString remark;
    QString createdAt;
};

struct Vehicle {
    int id = -1;
    QString plateNo;
    QString energyType;    // 燃油 / 新能源
    QString vehicleType;   // 货车 / 客车
    QString vehicleLevel;  // 轿式货车（4M以下） / 小型客车（9人及以下） ...
    QString kind;          // 常用车 / 临时车
    double load = 0;       // 载重（吨）
    int driverId = -1;
    QString driverName;
    QString remark;
    QString createdAt;
};

struct Device {
    int id = -1;
    QString serial;        // 探测仪序号
    QString type;          // 手持车辆探测仪 / 地面传感器
    bool online = false;
    QDateTime lastSeen;
    bool removed = false;  // 离线“移除”后置 true，下次上线自动恢复
    QString remark;
};

struct Detection {
    int id = -1;
    QString no;            // 检测编号
    QString deviceSerial;  // 探测仪序号
    QString detector;      // 检测人
    QDateTime time;
    bool passed = true;    // 检测结果
    QString plateNo;
    QString driverName;
    QString vehicleType;
    QString category;      // 类别
    double load = 0;
    int passengers = 0;
    QString reason;        // 进出事由
    QString remark;
    // 新增参数展示（批注优化点2）
    double temperature = 0;
    double humidity = 0;
    double speed = 0;
    int axleCount = 0;
};

struct LicenseInfo {
    QString customer;
    QString licenseData;   // 密文
    QDate expireDate;
    QDate issuedDate;
    QString plainSummary;  // 解密后的摘要（仅显示）
};

struct OperationLog {
    int id = -1;
    QString user;
    QString action;
    QString detail;
    QDateTime time;
};

#endif // CORE_MODELS_H

#include "accountpage.h"
#include "core/database.h"
#include "core/session.h"
#include "core/crypto.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QDateTime>
#include <QGroupBox>
#include <QStringList>

AccountPage::AccountPage(QWidget* parent)
    : IPage(parent)
{
    setupUi();
    refresh();
}

void AccountPage::setupUi()
{
    auto* root = new QVBoxLayout(this);
    auto* addBtn = new QPushButton(QStringLiteral("新增账号"));
    auto* editBtn = new QPushButton(QStringLiteral("编辑/分配权限"));
    auto* removeBtn = new QPushButton(QStringLiteral("删除"));
    auto* toggleBtn = new QPushButton(QStringLiteral("启用/停用"));
    connect(addBtn, &QPushButton::clicked, this, &AccountPage::onAdd);
    connect(editBtn, &QPushButton::clicked, this, &AccountPage::onEdit);
    connect(removeBtn, &QPushButton::clicked, this, &AccountPage::onRemove);
    connect(toggleBtn, &QPushButton::clicked, this, &AccountPage::onToggleEnabled);

    auto* top = new QHBoxLayout;
    top->addWidget(addBtn);
    top->addWidget(editBtn);
    top->addWidget(removeBtn);
    top->addWidget(toggleBtn);
    top->addStretch();

    m_table = new QTableWidget(0, 6);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("用户名"), QStringLiteral("姓名"), QStringLiteral("角色"),
        QStringLiteral("权限"), QStringLiteral("状态"), QStringLiteral("创建时间")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    root->addLayout(top);
    root->addWidget(m_table, 1);
}

QString permissionNames(Permissions p)
{
    QStringList names;
    if (p.testFlag(Permission::DetectionMode))   names << QStringLiteral("检测模式");
    if (p.testFlag(Permission::DetectionRecord)) names << QStringLiteral("检测记录");
    if (p.testFlag(Permission::DeviceManage))    names << QStringLiteral("设备管理");
    if (p.testFlag(Permission::AccountManage))   names << QStringLiteral("账户管理");
    if (p.testFlag(Permission::DriverManage))    names << QStringLiteral("司机管理");
    if (p.testFlag(Permission::VehicleManage))   names << QStringLiteral("车辆管理");
    if (p.testFlag(Permission::LicenseManage))   names << QStringLiteral("授权管理");
    if (p.testFlag(Permission::OperationLog))    names << QStringLiteral("操作日志");
    if (p.testFlag(Permission::Settings))        names << QStringLiteral("参数配置");
    if (p.testFlag(Permission::IntervalSetting)) names << QStringLiteral("间隔设置");
    return names.join(QStringLiteral("、"));
}

void AccountPage::refresh()
{
    const QVector<Account> list = Database::instance().allAccounts();
    m_table->setRowCount(list.size());
    for (int i = 0; i < list.size(); ++i) {
        const Account& a = list[i];
        const QStringList cells = {
            a.username, a.realName, roleToString(a.role),
            a.role == Role::Admin ? QStringLiteral("全部") : permissionNames(a.permissions),
            a.enabled ? QStringLiteral("启用") : QStringLiteral("停用"), a.createdAt};
        for (int col = 0; col < cells.size(); ++col) {
            auto* item = new QTableWidgetItem(cells.at(col));
            item->setData(Qt::UserRole, a.id);
            m_table->setItem(i, col, item);
        }
    }
}

void AccountPage::openEditor(int id)
{
    Account existing;
    if (id >= 0) {
        for (const Account& a : Database::instance().allAccounts()) {
            if (a.id == id) { existing = a; break; }
        }
    }

    QDialog dialog(this);
    dialog.setWindowTitle(id < 0 ? QStringLiteral("新增账号") : QStringLiteral("编辑账号与权限"));
    auto* layout = new QVBoxLayout(&dialog);
    auto* form = new QFormLayout;
    auto* username = new QLineEdit(existing.username);
    auto* realName = new QLineEdit(existing.realName);
    auto* password = new QLineEdit;
    password->setEchoMode(QLineEdit::Password);
    password->setPlaceholderText(id < 0 ? QStringLiteral("请输入密码") : QStringLiteral("留空则不修改"));
    auto* role = new QComboBox;
    role->addItem(QStringLiteral("admin"), static_cast<int>(Role::Admin));
    role->addItem(QStringLiteral("管理员"), static_cast<int>(Role::Manager));
    role->addItem(QStringLiteral("普通用户"), static_cast<int>(Role::User));
    role->setCurrentIndex(static_cast<int>(existing.role));
    form->addRow(QStringLiteral("用户名"), username);
    form->addRow(QStringLiteral("姓名"), realName);
    form->addRow(QStringLiteral("密码"), password);
    form->addRow(QStringLiteral("角色"), role);
    layout->addLayout(form);

    // 权限分配（批注3：包含“参数配置”）
    auto* permGroup = new QGroupBox(QStringLiteral("分配权限"));
    auto* permLayout = new QVBoxLayout(permGroup);
    struct P { Permission flag; const char* label; };
    const P perms[] = {
        {Permission::DetectionMode,   "检测模式"},
        {Permission::DetectionRecord, "检测记录"},
        {Permission::DeviceManage,    "设备管理"},
        {Permission::AccountManage,   "账户管理"},
        {Permission::DriverManage,    "司机管理"},
        {Permission::VehicleManage,   "车辆管理"},
        {Permission::LicenseManage,   "授权管理"},
        {Permission::OperationLog,    "操作日志"},
        {Permission::Settings,        "参数配置"},
        {Permission::IntervalSetting, "间隔设置"},
    };
    QVector<QCheckBox*> checks;
    for (const P& p : perms) {
        auto* cb = new QCheckBox(QString::fromUtf8(p.label));
        cb->setChecked(existing.permissions.testFlag(p.flag));
        // 批注3：普通用户默认权限不含“参数配置”，且默认只有检测记录和间隔设置
        if (id < 0 && role->currentIndex() == static_cast<int>(Role::User)) {
            cb->setChecked(p.flag == Permission::DetectionRecord || p.flag == Permission::IntervalSetting);
        }
        permLayout->addWidget(cb);
        checks.append(cb);
    }
    layout->addWidget(permGroup);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) return;
    if (username->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("用户名不能为空"));
        return;
    }
    if (id < 0 && password->text().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("密码不能为空"));
        return;
    }

    Account a = existing;
    a.username = username->text().trimmed();
    a.realName = realName->text().trimmed();
    a.role = static_cast<Role>(role->currentData().toInt());
    a.permissions = Permissions();
    for (int i = 0; i < checks.size(); ++i) {
        if (checks.at(i)->isChecked()) {
            const Permission flags[] = {
                Permission::DetectionMode, Permission::DetectionRecord, Permission::DeviceManage,
                Permission::AccountManage, Permission::DriverManage, Permission::VehicleManage,
                Permission::LicenseManage, Permission::OperationLog, Permission::Settings,
                Permission::IntervalSetting};
            a.permissions |= flags[i];
        }
    }
    if (!password->text().isEmpty()) {
        a.salt = Crypto::makeSalt();
        a.passwordHash = Crypto::passwordHash(password->text(), a.salt);
    }

    if (id < 0) {
        if (a.salt.isEmpty()) {
            a.salt = Crypto::makeSalt();
            a.passwordHash = Crypto::passwordHash(password->text(), a.salt);
        }
        a.enabled = true;
        a.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);
        Database::instance().createAccount(a);
    } else {
        Database::instance().updateAccount(a);
    }
    Database::instance().log(Session::instance().currentAccount().username,
                             id < 0 ? QStringLiteral("新增账号") : QStringLiteral("编辑账号"), a.username);
    refresh();
}

void AccountPage::onAdd() { openEditor(-1); }

void AccountPage::onEdit()
{
    const int row = m_table->currentRow();
    if (row < 0) { QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择一行")); return; }
    openEditor(m_table->item(row, 0)->data(Qt::UserRole).toInt());
}

void AccountPage::onRemove()
{
    const int row = m_table->currentRow();
    if (row < 0) { QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择一行")); return; }
    const int id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    const QString name = m_table->item(row, 0)->text();
    if (name == QStringLiteral("admin")) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("admin 账号不可删除"));
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("确认"), QStringLiteral("确定删除该账号吗？"),
                             QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    Database::instance().deleteAccount(id);
    Database::instance().log(Session::instance().currentAccount().username,
                             QStringLiteral("删除账号"), name);
    refresh();
}

void AccountPage::onToggleEnabled()
{
    const int row = m_table->currentRow();
    if (row < 0) { QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择一行")); return; }
    const int id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    Account a = Database::instance().accountById(id);
    if (a.username == QStringLiteral("admin")) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("admin 账号不可停用"));
        return;
    }
    a.enabled = !a.enabled;
    Database::instance().updateAccount(a);
    refresh();
}

void AccountPage::onEnter() { refresh(); }

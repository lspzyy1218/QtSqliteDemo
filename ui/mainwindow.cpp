#include "mainwindow.h"
#include "ui/ipage.h"
#include "ui/detectionpage.h"
#include "ui/detectionrecordpage.h"
#include "ui/devicepage.h"
#include "ui/accountpage.h"
#include "ui/driverpage.h"
#include "ui/vehiclepage.h"
#include "ui/licensepage.h"
#include "ui/logpage.h"
#include "ui/settingspage.h"
#include "ui/intervalpage.h"
#include "core/session.h"
#include "core/database.h"

#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QToolBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QListWidgetItem>
#include <QApplication>

struct MenuEntry {
    const char* title;
    Permission perm;
    int pageIndex; // 对应 m_stack 顺序，-1 表示检测模式特殊处理
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUi();
    setWindowTitle(QStringLiteral("智能车辆身份检测系统 - 上位机"));
    resize(1180, 720);
    refreshMenu();
}

void MainWindow::setupUi()
{
    // 先添加所有页面（顺序与 MenuEntry 的 pageIndex 对应）
    addPage(new DetectionPage(this));          // 0 检测模式
    addPage(new DetectionRecordPage(this));    // 1 检测记录
    addPage(new DevicePage(this));             // 2 设备管理
    addPage(new AccountPage(this));            // 3 账户管理
    addPage(new DriverPage(this));             // 4 司机管理
    addPage(new VehiclePage(this));            // 5 车辆管理
    addPage(new LicensePage(this));            // 6 授权管理
    addPage(new LogPage(this));                // 7 操作日志
    addPage(new SettingsPage(this));           // 8 参数配置
    addPage(new IntervalPage(this));           // 9 间隔设置

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // 左侧菜单
    m_menu = new QListWidget;
    m_menu->setFixedWidth(180);
    m_menu->setStyleSheet(QStringLiteral(
        "QListWidget{background:#f5f7fa;border:none;font-size:14px;}"
        "QListWidget::item{height:38px;padding-left:18px;}"
        "QListWidget::item:selected{background:#1a6fb5;color:white;}"));
    connect(m_menu, &QListWidget::currentRowChanged, this, &MainWindow::onMenuClicked);

    // 右侧页面
    m_stack = new QStackedWidget;

    root->addWidget(m_menu);
    root->addWidget(m_stack, 1);

    // 顶部工具栏
    auto* toolbar = new QToolBar(QStringLiteral("工具栏"), this);
    toolbar->setMovable(false);
    m_userLabel = new QLabel;
    m_licenseLabel = new QLabel;
    auto* logoutBtn = new QPushButton(QStringLiteral("退出登录"));
    connect(logoutBtn, &QPushButton::clicked, this, &MainWindow::onLogout);
    toolbar->addWidget(m_userLabel);
    toolbar->addWidget(m_licenseLabel);
    toolbar->addWidget(logoutBtn);
    addToolBar(toolbar);
}

void MainWindow::addPage(IPage* page)
{
    m_stack->addWidget(page);
}

void MainWindow::refreshMenu()
{
    static const MenuEntry entries[] = {
        {"检测模式",   Permission::DetectionMode,   0},
        {"检测记录",   Permission::DetectionRecord, 1},
        {"设备管理",   Permission::DeviceManage,    2},
        {"账户管理",   Permission::AccountManage,   3},
        {"司机管理",   Permission::DriverManage,    4},
        {"车辆管理",   Permission::VehicleManage,   5},
        {"授权管理",   Permission::LicenseManage,   6},
        {"操作日志",   Permission::OperationLog,    7},
        {"参数配置",   Permission::Settings,        8},
        {"间隔设置",   Permission::IntervalSetting, 9},
    };

    m_menu->clear();
    for (const MenuEntry& e : entries) {
        if (!Session::instance().has(e.perm)) {
            continue; // 批注3：按权限显示菜单
        }
        auto* item = new QListWidgetItem(QString::fromUtf8(e.title));
        item->setData(Qt::UserRole, e.pageIndex);
        m_menu->addItem(item);
    }

    const Account& acc = Session::instance().currentAccount();
    m_userLabel->setText(QStringLiteral(" 当前用户：%1（%2） ")
                         .arg(acc.realName.isEmpty() ? acc.username : acc.realName)
                         .arg(roleToString(acc.role)));
    m_licenseLabel->setText(QStringLiteral("  %1  ").arg(Session::instance().licenseExpireText()));

    if (m_menu->count() > 0) {
        m_menu->setCurrentRow(0);
    }
}

void MainWindow::onMenuClicked(int row)
{
    if (row < 0 || row >= m_menu->count()) {
        return;
    }
    const int pageIndex = m_menu->item(row)->data(Qt::UserRole).toInt();

    // 批注5：进入单车/多车模式前校验授权
    if (pageIndex == 0 && !Session::instance().isLicenseValid()) {
        showLicenseExpiredDialog();
        m_menu->setCurrentRow(1 >= m_menu->count() ? 0 : 1);
        return;
    }

    m_stack->setCurrentIndex(pageIndex);
    if (auto* page = qobject_cast<IPage*>(m_stack->widget(pageIndex))) {
        page->onEnter();
    }
}

void MainWindow::showLicenseExpiredDialog()
{
    auto* box = new QMessageBox(this);
    box->setIcon(QMessageBox::Warning);
    box->setWindowTitle(QStringLiteral("授权校验"));
    box->setText(QStringLiteral("未授权或授权已到期，请先导入授权文件。"));
    auto* licenseBtn = box->addButton(QStringLiteral("授权"), QMessageBox::AcceptRole);
    box->addButton(QStringLiteral("取消"), QMessageBox::RejectRole);
    box->exec();
    if (box->clickedButton() == licenseBtn) {
        for (int i = 0; i < m_menu->count(); ++i) {
            if (m_menu->item(i)->data(Qt::UserRole).toInt() == 6) {
                m_menu->setCurrentRow(i);
                break;
            }
        }
    }
    box->deleteLater();
}

void MainWindow::onLogout()
{
    const auto ret = QMessageBox::question(this, QStringLiteral("确认"),
                                          QStringLiteral("确定要退出登录吗？"),
                                          QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        Database::instance().log(Session::instance().currentAccount().username,
                                 QStringLiteral("退出"), QStringLiteral("用户退出登录"));
        Session::instance().clear();
        close();
    }
}

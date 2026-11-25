#include "settingspage.h"
#include "core/database.h"
#include "core/session.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QMessageBox>
#include <QApplication>
#include <QVector>

SettingsPage::SettingsPage(QWidget* parent)
    : IPage(parent)
{
    setupUi();
    refresh();
}

void SettingsPage::setupUi()
{
    auto* root = new QVBoxLayout(this);
    const bool isAdmin = Session::instance().role() == Role::Admin;

    auto* basicGroup = new QGroupBox(QStringLiteral("基础信息"));
    auto* basicForm = new QFormLayout(basicGroup);
    struct Item { const char* key; const char* label; const char* cat; };
    const Item basics[] = {
        {"device_detector", "手持车辆探测仪名称", "basic"},
        {"device_sensor",   "地面传感器名称",     "basic"},
    };
    for (const Item& it : basics) {
        Field f;
        f.key = QString::fromLatin1(it.key);
        f.category = QString::fromLatin1(it.cat);
        f.edit = new QLineEdit;
        f.edit->setEnabled(isAdmin); // 基础信息：管理员/普通用户也可改（批注8）
        basicForm->addRow(QString::fromUtf8(it.label), f.edit);
        m_fields.append(f);
    }

    auto* detectGroup = new QGroupBox(QStringLiteral("检测参数"));
    auto* detectForm = new QFormLayout(detectGroup);
    const Item detects[] = {
        {"detect_mode",       "默认检测模式", "detect"},
        {"detect_timeout",    "检测超时（秒）", "detect"},
    };
    for (const Item& it : detects) {
        Field f;
        f.key = QString::fromLatin1(it.key);
        f.category = QString::fromLatin1(it.cat);
        f.edit = new QLineEdit;
        f.edit->setEnabled(true);
        detectForm->addRow(QString::fromUtf8(it.label), f.edit);
        m_fields.append(f);
    }
    Field newParams;
    newParams.key = QStringLiteral("detect_new_params");
    newParams.category = QStringLiteral("detect");
    newParams.check = new QCheckBox(QStringLiteral("显示新参数（温湿度/速度/轴数）"));
    detectForm->addRow(newParams.check);
    m_fields.append(newParams);

    Field chart;
    chart.key = QStringLiteral("show_chart");
    chart.category = QStringLiteral("detect");
    chart.check = new QCheckBox(QStringLiteral("显示检测结果图表"));
    detectForm->addRow(chart.check);
    m_fields.append(chart);

    auto* advancedGroup = new QGroupBox(QStringLiteral("高级设置（仅 admin，部分需重启）"));
    auto* advancedForm = new QFormLayout(advancedGroup);
    const Item advanced[] = {
        {"server_port", "服务端口（需重启）", "advanced"},
        {"log_level",   "日志级别",           "advanced"},
    };
    for (const Item& it : advanced) {
        Field f;
        f.key = QString::fromLatin1(it.key);
        f.category = QString::fromLatin1(it.cat);
        f.edit = new QLineEdit;
        f.edit->setEnabled(isAdmin); // 仅 admin 可改
        advancedForm->addRow(QString::fromUtf8(it.label), f.edit);
        m_fields.append(f);
    }

    auto* saveBtn = new QPushButton(QStringLiteral("保存配置"));
    connect(saveBtn, &QPushButton::clicked, this, &SettingsPage::onSave);
    if (!isAdmin) {
        advancedGroup->setTitle(QStringLiteral("高级设置（仅 admin 可修改）"));
    }

    auto* note = new QLabel(QStringLiteral(
        "说明：标记“需重启”的参数保存后会提示重启应用；高级设置仅 admin 可修改。"));
    note->setWordWrap(true);
    note->setStyleSheet(QStringLiteral("color:#888;"));

    root->addWidget(basicGroup);
    root->addWidget(detectGroup);
    root->addWidget(advancedGroup);
    root->addWidget(note);
    root->addWidget(saveBtn);
    root->addStretch();
}

void SettingsPage::refresh()
{
    for (const Field& f : m_fields) {
        const QString value = Database::instance().setting(f.key);
        if (f.edit) f.edit->setText(value);
        if (f.check) f.check->setChecked(value == QStringLiteral("true"));
    }
}

void SettingsPage::onSave()
{
    bool needRestart = false;
    for (const Field& f : m_fields) {
        QString value;
        if (f.edit) value = f.edit->text();
        if (f.check) value = f.check->isChecked() ? QStringLiteral("true") : QStringLiteral("false");
        Database::instance().setSetting(f.key, value);
        if (Database::instance().settingRestartRequired(f.key) && !needRestart) {
            needRestart = true;
        }
    }
    Database::instance().log(Session::instance().currentAccount().username,
                             QStringLiteral("保存配置"), QStringLiteral("修改系统参数"));

    if (needRestart) {
        const auto ret = QMessageBox::question(this, QStringLiteral("需要重启"),
            QStringLiteral("部分修改需重启应用才能生效，是否立即重启？"),
            QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            Database::instance().log(Session::instance().currentAccount().username,
                                     QStringLiteral("重启应用"), QString());
            QApplication::quit();
            return;
        }
    }
    QMessageBox::information(this, QStringLiteral("完成"), QStringLiteral("配置已保存"));
    refresh();
}

void SettingsPage::onEnter()
{
    // 页面在登录前构造，进入时按当前角色重新设置可编辑状态
    const bool isAdmin = Session::instance().role() == Role::Admin;
    for (const Field& f : m_fields) {
        if (!f.edit) continue;
        if (f.category == QStringLiteral("advanced")) {
            f.edit->setEnabled(isAdmin);
        } else {
            f.edit->setEnabled(true);
        }
    }
    refresh();
}

#include "loginwidget.h"
#include "core/database.h"
#include "core/session.h"
#include "core/crypto.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QApplication>

LoginWidget::LoginWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    setWindowTitle(QStringLiteral("智能车辆检测系统 - 登录"));
    resize(420, 260);
}

void LoginWidget::setupUi()
{
    setStyleSheet(QStringLiteral("#loginCard{background:#ffffff;border-radius:12px;}"));

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(48, 36, 48, 36);

    auto* card = new QWidget;
    card->setObjectName(QStringLiteral("loginCard"));
    auto* root = new QVBoxLayout(card);
    root->setContentsMargins(36, 32, 36, 32);
    root->setSpacing(14);

    auto* logo = new QLabel(QStringLiteral("◆"));
    logo->setAlignment(Qt::AlignCenter);
    logo->setStyleSheet(QStringLiteral("font-size:42px;color:#409eff;"));

    auto* title = new QLabel(QStringLiteral("智能车辆身份检测系统"));
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral("font-size:20px;font-weight:bold;color:#2c3e50;"));

    auto* subtitle = new QLabel(QStringLiteral("车辆检测上位机 · 登录"));
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet(QStringLiteral("color:#909399;font-size:12px;"));

    m_userEdit = new QLineEdit;
    m_userEdit->setPlaceholderText(QStringLiteral("请输入用户名"));
    m_userEdit->setFixedHeight(38);
    m_passEdit = new QLineEdit;
    m_passEdit->setPlaceholderText(QStringLiteral("请输入密码"));
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passEdit->setFixedHeight(38);

    auto* form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(14);
    form->addRow(QStringLiteral("账号"), m_userEdit);
    form->addRow(QStringLiteral("密码"), m_passEdit);

    auto* loginBtn = new QPushButton(QStringLiteral("登 录"));
    loginBtn->setFixedHeight(40);
    loginBtn->setStyleSheet(QStringLiteral(
        "QPushButton{background:#409eff;color:white;border-radius:6px;font-size:15px;font-weight:bold;}"
        "QPushButton:hover{background:#66b1ff;}QPushButton:pressed{background:#337ecc;}"));
    connect(loginBtn, &QPushButton::clicked, this, &LoginWidget::onLogin);
    connect(m_passEdit, &QLineEdit::returnPressed, this, &LoginWidget::onLogin);

    m_hint = new QLabel(QStringLiteral("初始管理员：admin / Admin@2026!Strong"));
    m_hint->setAlignment(Qt::AlignCenter);
    m_hint->setStyleSheet(QStringLiteral("color:#a8abb2;font-size:11px;"));
    m_hint->setWordWrap(true);

    root->addWidget(logo);
    root->addWidget(title);
    root->addWidget(subtitle);
    root->addSpacing(10);
    root->addLayout(form);
    root->addSpacing(6);
    root->addWidget(loginBtn);
    root->addWidget(m_hint);

    outer->addWidget(card);
}

void LoginWidget::onLogin()
{
    const QString username = m_userEdit->text().trimmed();
    const QString password = m_passEdit->text();
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入账号和密码"));
        return;
    }

    const Account account = Database::instance().accountByUsername(username);
    if (account.id < 0 || !account.enabled) {
        QMessageBox::warning(this, QStringLiteral("登录失败"), QStringLiteral("账号不存在或已停用"));
        return;
    }
    const QString hash = Crypto::passwordHash(password, account.salt);
    if (hash != account.passwordHash) {
        QMessageBox::warning(this, QStringLiteral("登录失败"), QStringLiteral("密码错误"));
        return;
    }

    Session::instance().setAccount(account);
    Database::instance().log(account.username, QStringLiteral("登录"), QStringLiteral("用户登录系统"));
    emit loginSucceeded();
}

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
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(40, 30, 40, 30);

    auto* title = new QLabel(QStringLiteral("智能车辆身份检测系统"));
    title->setStyleSheet(QStringLiteral("font-size:20px;font-weight:bold;color:#1a6fb5;"));
    title->setAlignment(Qt::AlignCenter);

    m_userEdit = new QLineEdit;
    m_userEdit->setPlaceholderText(QStringLiteral("用户名"));
    m_passEdit = new QLineEdit;
    m_passEdit->setPlaceholderText(QStringLiteral("密码"));
    m_passEdit->setEchoMode(QLineEdit::Password);

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("账号"), m_userEdit);
    form->addRow(QStringLiteral("密码"), m_passEdit);

    auto* loginBtn = new QPushButton(QStringLiteral("登录"));
    loginBtn->setStyleSheet(QStringLiteral("QPushButton{background:#1a6fb5;color:white;padding:8px;}"));
    connect(loginBtn, &QPushButton::clicked, this, &LoginWidget::onLogin);
    connect(m_passEdit, &QLineEdit::returnPressed, this, &LoginWidget::onLogin);

    m_hint = new QLabel(QStringLiteral("初始管理员：admin / Admin@2026!Strong"));
    m_hint->setStyleSheet(QStringLiteral("color:#888;font-size:11px;"));
    m_hint->setWordWrap(true);

    root->addWidget(title);
    root->addSpacing(20);
    root->addLayout(form);
    root->addSpacing(10);
    root->addWidget(loginBtn);
    root->addSpacing(10);
    root->addWidget(m_hint);
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

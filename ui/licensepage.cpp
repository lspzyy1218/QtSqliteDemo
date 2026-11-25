#include "licensepage.h"
#include "core/database.h"
#include "core/crypto.h"
#include "core/session.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDateEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDate>

LicensePage::LicensePage(QWidget* parent)
    : IPage(parent)
{
    setupUi();
    refresh();
}

void LicensePage::setupUi()
{
    auto* root = new QVBoxLayout(this);
    m_status = new QLabel;
    m_status->setStyleSheet(QStringLiteral("font-size:14px;font-weight:bold;padding:8px;"));

    m_customer = new QLineEdit;
    m_cipher = new QLineEdit;
    m_cipher->setReadOnly(true);
    m_cipher->setPlaceholderText(QStringLiteral("授权信息密文（不可手工修改）"));
    m_expire = new QDateEdit;
    m_expire->setCalendarPopup(true);
    m_expire->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_expire->setDate(QDate::currentDate().addYears(1));

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("客户名称"), m_customer);
    form->addRow(QStringLiteral("授权到期日"), m_expire);
    form->addRow(QStringLiteral("授权信息（密文）"), m_cipher);

    auto* importBtn = new QPushButton(QStringLiteral("导入授权文件"));
    auto* generateBtn = new QPushButton(QStringLiteral("生成授权文件"));
    auto* applyBtn = new QPushButton(QStringLiteral("应用授权"));
    connect(importBtn, &QPushButton::clicked, this, &LicensePage::onImport);
    connect(generateBtn, &QPushButton::clicked, this, &LicensePage::onGenerateFile);
    connect(applyBtn, &QPushButton::clicked, this, &LicensePage::onApply);

    auto* btnLayout = new QHBoxLayout;
    btnLayout->addWidget(importBtn);
    btnLayout->addWidget(generateBtn);
    btnLayout->addWidget(applyBtn);
    btnLayout->addStretch();

    root->addWidget(m_status);
    root->addLayout(form);
    root->addLayout(btnLayout);
    root->addStretch();
}

void LicensePage::refresh()
{
    const LicenseInfo info = Database::instance().loadLicense();
    m_customer->setText(info.customer);
    m_cipher->setText(info.licenseData);
    if (info.expireDate.isValid()) m_expire->setDate(info.expireDate);

    const bool valid = Session::instance().isLicenseValid();
    m_status->setText(valid
        ? QStringLiteral("授权有效：%1").arg(Session::instance().licenseExpireText())
        : QStringLiteral("未授权或授权已到期"));
    m_status->setStyleSheet(valid
        ? QStringLiteral("color:#2e8b57;font-weight:bold;padding:8px;background:#eafbe7;")
        : QStringLiteral("color:#c0392b;font-weight:bold;padding:8px;background:#fdecea;"));
}

void LicensePage::onGenerateFile()
{
    if (m_customer->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请填写客户名称"));
        return;
    }
    const QString plain = QStringLiteral("customer=%1;expire=%2;product=JT-Xin-PeTing;version=1.0")
        .arg(m_customer->text().trimmed())
        .arg(m_expire->date().toString(Qt::ISODate));
    const QString cipher = Crypto::encrypt(plain, Crypto::licenseKey());

    LicenseInfo info;
    info.customer = m_customer->text().trimmed();
    info.licenseData = cipher;
    info.expireDate = m_expire->date();
    info.issuedDate = QDate::currentDate();
    info.plainSummary = plain;

    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("生成授权文件"),
        QStringLiteral("license.dat"), QStringLiteral("授权文件 (*.dat *.lic *.txt)"));
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法写入授权文件"));
        return;
    }
    QTextStream out(&file);
    out << cipher;
    file.close();

    Database::instance().saveLicense(info);
    Database::instance().log(Session::instance().currentAccount().username,
                             QStringLiteral("生成授权"), info.customer);
    refresh();
    QMessageBox::information(this, QStringLiteral("完成"), QStringLiteral("授权文件已生成并应用"));
}

void LicensePage::onImport()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("导入授权文件"),
        QString(), QStringLiteral("授权文件 (*.dat *.lic *.txt)"));
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法读取授权文件"));
        return;
    }
    QTextStream in(&file);
    const QString cipher = in.readAll().trimmed();
    file.close();

    const QString plain = Crypto::decrypt(cipher, Crypto::licenseKey());
    // 批注5-3：密文不能轻易破解修改，解密失败则拒绝导入
    if (!plain.contains(QStringLiteral("customer=")) || !plain.contains(QStringLiteral("expire="))) {
        QMessageBox::critical(this, QStringLiteral("导入失败"), QStringLiteral("授权文件格式无效或已被篡改"));
        return;
    }

    LicenseInfo info;
    info.customer = plain.section(QStringLiteral("customer="), 1).section(';', 0, 0);
    const QString expire = plain.section(QStringLiteral("expire="), 1).section(';', 0, 0);
    info.expireDate = QDate::fromString(expire, Qt::ISODate);
    info.issuedDate = QDate::currentDate();
    info.licenseData = cipher;
    info.plainSummary = plain;

    if (!info.expireDate.isValid()) {
        QMessageBox::critical(this, QStringLiteral("导入失败"), QStringLiteral("授权有效期无效"));
        return;
    }
    Database::instance().saveLicense(info);
    Database::instance().log(Session::instance().currentAccount().username,
                             QStringLiteral("导入授权"), info.customer);
    refresh();
    QMessageBox::information(this, QStringLiteral("完成"),
                             QStringLiteral("授权导入成功，有效期至 %1").arg(info.expireDate.toString(Qt::ISODate)));
}

void LicensePage::onApply()
{
    refresh();
}

void LicensePage::onEnter() { refresh(); }

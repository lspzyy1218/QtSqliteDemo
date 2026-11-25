#include "intervalpage.h"
#include "core/database.h"
#include "core/session.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QMessageBox>
#include <QApplication>

IntervalPage::IntervalPage(QWidget* parent)
    : IPage(parent)
{
    setupUi();
    onEnter();
}

void IntervalPage::setupUi()
{
    auto* root = new QVBoxLayout(this);
    m_spin = new QSpinBox;
    m_spin->setRange(1, 3600);
    m_spin->setSuffix(QStringLiteral(" 秒"));

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("重新初始化检测页面间隔"), m_spin);

    auto* note = new QLabel(QStringLiteral("说明：设置重新初始化检测页面的间隔时间"));
    note->setStyleSheet(QStringLiteral("color:#666;"));

    m_hint = new QLabel;
    m_hint->setWordWrap(true);
    m_hint->setStyleSheet(QStringLiteral("color:#888;background:#f5f7fa;padding:8px;border-radius:4px;"));

    auto* saveBtn = new QPushButton(QStringLiteral("保存"));
    auto* restoreBtn = new QPushButton(QStringLiteral("恢复默认设置"));
    connect(saveBtn, &QPushButton::clicked, this, &IntervalPage::onSave);
    connect(restoreBtn, &QPushButton::clicked, this, &IntervalPage::onRestore);

    auto* btnLayout = new QHBoxLayout;
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(restoreBtn);
    btnLayout->addStretch();

    root->addLayout(form);
    root->addWidget(note);
    root->addSpacing(20);
    root->addWidget(new QLabel(QStringLiteral("界面恢复设置说明")));
    root->addWidget(m_hint);
    root->addLayout(btnLayout);
    root->addStretch();
}

void IntervalPage::onSave()
{
    Database::instance().setSetting(QStringLiteral("reinit_interval"), QString::number(m_spin->value()));
    Database::instance().log(Session::instance().currentAccount().username,
                             QStringLiteral("保存间隔设置"), QString::number(m_spin->value()) + QStringLiteral("秒"));
    const auto ret = QMessageBox::question(this, QStringLiteral("需要重启"),
        QStringLiteral("间隔设置需重启应用才能生效，是否立即重启？"),
        QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        QApplication::quit();
        return;
    }
    QMessageBox::information(this, QStringLiteral("完成"), QStringLiteral("已保存，重启后生效"));
}

void IntervalPage::onRestore()
{
    if (QMessageBox::question(this, QStringLiteral("确认恢复"),
        QStringLiteral("确定恢复默认设置吗？所有参数将还原为出厂值，且需重启应用生效。"),
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

    Database::instance().setSetting(QStringLiteral("reinit_interval"), QStringLiteral("8"));
    Database::instance().log(Session::instance().currentAccount().username,
                             QStringLiteral("恢复默认设置"), QString());
    m_spin->setValue(8);
    QMessageBox::information(this, QStringLiteral("完成"), QStringLiteral("已恢复默认设置，请重启应用"));
}

void IntervalPage::onEnter()
{
    m_spin->setValue(Database::instance().setting(QStringLiteral("reinit_interval"), QStringLiteral("8")).toInt());
    m_hint->setText(Database::instance().setting(QStringLiteral("restore_hint"),
        QStringLiteral("恢复默认设置后，所有参数将还原为出厂值，且需重启应用生效")));
}

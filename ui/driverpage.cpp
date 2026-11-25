#include "driverpage.h"
#include "core/database.h"
#include "core/session.h"

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
#include <QMessageBox>
#include <QDateTime>

DriverPage::DriverPage(QWidget* parent)
    : IPage(parent)
{
    setupUi();
    refresh();
}

void DriverPage::setupUi()
{
    auto* root = new QVBoxLayout(this);
    auto* addBtn = new QPushButton(QStringLiteral("新增司机"));
    auto* editBtn = new QPushButton(QStringLiteral("编辑"));
    auto* removeBtn = new QPushButton(QStringLiteral("删除"));
    connect(addBtn, &QPushButton::clicked, this, &DriverPage::onAdd);
    connect(editBtn, &QPushButton::clicked, this, &DriverPage::onEdit);
    connect(removeBtn, &QPushButton::clicked, this, &DriverPage::onRemove);

    auto* top = new QHBoxLayout;
    top->addWidget(addBtn);
    top->addWidget(editBtn);
    top->addWidget(removeBtn);
    top->addStretch();

    m_table = new QTableWidget(0, 5);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("姓名"), QStringLiteral("电话"), QStringLiteral("身份证号"),
        QStringLiteral("备注"), QStringLiteral("创建日期")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    root->addLayout(top);
    root->addWidget(m_table, 1);
}

void DriverPage::refresh()
{
    const QVector<Driver> list = Database::instance().allDrivers();
    m_table->setRowCount(list.size());
    for (int i = 0; i < list.size(); ++i) {
        const Driver& d = list[i];
        const QStringList cells = {d.name, d.phone, d.idCard, d.remark, d.createdAt};
        for (int col = 0; col < cells.size(); ++col) {
            auto* item = new QTableWidgetItem(cells.at(col));
            item->setData(Qt::UserRole, d.id);
            m_table->setItem(i, col, item);
        }
    }
}

void DriverPage::openEditor(int id)
{
    Driver existing;
    if (id >= 0) {
        for (const Driver& d : Database::instance().allDrivers()) {
            if (d.id == id) { existing = d; break; }
        }
    }

    QDialog dialog(this);
    dialog.setWindowTitle(id < 0 ? QStringLiteral("新增司机") : QStringLiteral("编辑司机"));
    auto* form = new QFormLayout(&dialog);
    auto* name = new QLineEdit(existing.name);
    auto* phone = new QLineEdit(existing.phone);
    auto* idCard = new QLineEdit(existing.idCard);
    auto* remark = new QLineEdit(existing.remark);
    form->addRow(QStringLiteral("姓名"), name);
    form->addRow(QStringLiteral("电话"), phone);
    form->addRow(QStringLiteral("身份证号"), idCard);
    form->addRow(QStringLiteral("备注"), remark);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form->addRow(buttons);

    if (dialog.exec() != QDialog::Accepted) return;
    if (name->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("姓名不能为空"));
        return;
    }
    Driver d = existing;
    d.name = name->text().trimmed();
    d.phone = phone->text().trimmed();
    d.idCard = idCard->text().trimmed();
    d.remark = remark->text().trimmed();
    if (id < 0) {
        d.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);
        Database::instance().createDriver(d);
    } else {
        Database::instance().updateDriver(d);
    }
    Database::instance().log(Session::instance().currentAccount().username,
                             id < 0 ? QStringLiteral("新增司机") : QStringLiteral("编辑司机"), d.name);
    refresh();
}

void DriverPage::onAdd() { openEditor(-1); }

void DriverPage::onEdit()
{
    const int row = m_table->currentRow();
    if (row < 0) { QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择一行")); return; }
    openEditor(m_table->item(row, 0)->data(Qt::UserRole).toInt());
}

void DriverPage::onRemove()
{
    const int row = m_table->currentRow();
    if (row < 0) { QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择一行")); return; }
    const int id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    if (QMessageBox::question(this, QStringLiteral("确认"), QStringLiteral("确定删除该司机吗？"),
                             QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    Database::instance().deleteDriver(id);
    Database::instance().log(Session::instance().currentAccount().username,
                             QStringLiteral("删除司机"), QString::number(id));
    refresh();
}

void DriverPage::onEnter() { refresh(); }

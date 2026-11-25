#include "devicepage.h"
#include "core/database.h"
#include "core/session.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QMessageBox>
#include <QDateTime>
#include <QInputDialog>
#include <QLineEdit>

DevicePage::DevicePage(QWidget* parent)
    : IPage(parent)
{
    setupUi();
    refresh();
}

void DevicePage::setupUi()
{
    auto* root = new QVBoxLayout(this);
    auto* simulateBtn = new QPushButton(QStringLiteral("模拟设备上线"));
    auto* removeBtn = new QPushButton(QStringLiteral("移除"));
    connect(simulateBtn, &QPushButton::clicked, this, &DevicePage::onSimulateOnline);
    connect(removeBtn, &QPushButton::clicked, this, &DevicePage::onRemove);

    auto* top = new QHBoxLayout;
    top->addWidget(new QLabel(QStringLiteral("设备列表（曾连接过上位机的设备均会保存）")));
    top->addStretch();
    top->addWidget(simulateBtn);
    top->addWidget(removeBtn);

    m_table = new QTableWidget(0, 5);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("设备序号"), QStringLiteral("设备类型"), QStringLiteral("状态"),
        QStringLiteral("最后在线时间"), QStringLiteral("备注")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    root->addLayout(top);
    root->addWidget(m_table, 1);
}

void DevicePage::refresh()
{
    // 批注1-2/3：显示在线和离线设备；被移除的设备默认不显示
    const QVector<Device> list = Database::instance().allDevices(false);
    m_table->setRowCount(list.size());
    for (int i = 0; i < list.size(); ++i) {
        const Device& d = list[i];
        const QStringList cells = {
            d.serial, d.type, d.online ? QStringLiteral("在线") : QStringLiteral("离线"),
            d.lastSeen.isValid() ? d.lastSeen.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")) : QString(),
            d.remark};
        for (int col = 0; col < cells.size(); ++col) {
            auto* item = new QTableWidgetItem(cells.at(col));
            item->setData(Qt::UserRole, d.id);
            if (col == 2) {
                item->setForeground(d.online ? QColor(46, 139, 87) : QColor(150, 150, 150));
            }
            m_table->setItem(i, col, item);
        }
    }
}

void DevicePage::onSimulateOnline()
{
    bool ok = false;
    const QString serial = QInputDialog::getText(this, QStringLiteral("模拟设备上线"),
        QStringLiteral("设备序号（已存在则恢复在线）"), QLineEdit::Normal, QStringLiteral("DT-001"), &ok);
    if (!ok || serial.trimmed().isEmpty()) return;

    Device d;
    d.serial = serial.trimmed();
    d.type = d.serial.contains(QStringLiteral("SENSOR")) ? QStringLiteral("地面传感器") : QStringLiteral("手持车辆探测仪");
    d.online = true;
    d.lastSeen = QDateTime::currentDateTime();
    d.remark = QStringLiteral("模拟上线");
    // upsert 会自动将 removed 置 0（批注1-3：下次上线后重新出现）
    Database::instance().upsertDevice(d);
    Database::instance().log(Session::instance().currentAccount().username,
                             QStringLiteral("设备上线"), d.serial);
    refresh();
}

void DevicePage::onRemove()
{
    const int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择一台设备"));
        return;
    }
    const int id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    if (QMessageBox::question(this, QStringLiteral("确认移除"),
        QStringLiteral("确定移除该设备吗？移除后将不在列表中显示，下次上线会重新出现。"),
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

    Database::instance().markDeviceRemoved(id, true);
    Database::instance().log(Session::instance().currentAccount().username,
                             QStringLiteral("移除设备"), QString::number(id));
    refresh();
}

void DevicePage::onEnter() { refresh(); }

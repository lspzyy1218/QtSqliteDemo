#include "vehiclepage.h"
#include "core/database.h"
#include "core/session.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QMessageBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QLabel>

VehiclePage::VehiclePage(QWidget* parent)
    : IPage(parent)
{
    setupUi();
    onQuery();
}

void VehiclePage::setupUi()
{
    auto* root = new QVBoxLayout(this);

    m_search = new QLineEdit;
    m_search->setPlaceholderText(QStringLiteral("按车牌号查询"));
    auto* queryBtn = new QPushButton(QStringLiteral("查询"));
    auto* resetBtn = new QPushButton(QStringLiteral("重置"));
    auto* addBtn = new QPushButton(QStringLiteral("新增车辆"));
    auto* editBtn = new QPushButton(QStringLiteral("编辑"));
    auto* removeBtn = new QPushButton(QStringLiteral("删除"));
    auto* associateBtn = new QPushButton(QStringLiteral("关联司机"));

    connect(queryBtn, &QPushButton::clicked, this, &VehiclePage::onQuery);
    connect(resetBtn, &QPushButton::clicked, [this]() { m_search->clear(); onQuery(); });
    connect(addBtn, &QPushButton::clicked, this, &VehiclePage::onAdd);
    connect(editBtn, &QPushButton::clicked, this, &VehiclePage::onEdit);
    connect(removeBtn, &QPushButton::clicked, this, &VehiclePage::onRemove);
    connect(associateBtn, &QPushButton::clicked, this, &VehiclePage::onAssociateDriver);

    auto* top = new QHBoxLayout;
    top->addWidget(new QLabel(QStringLiteral("车牌号：")));
    top->addWidget(m_search);
    top->addWidget(queryBtn);
    top->addWidget(resetBtn);
    top->addStretch();
    top->addWidget(addBtn);
    top->addWidget(editBtn);
    top->addWidget(removeBtn);
    top->addWidget(associateBtn);

    m_table = new QTableWidget(0, 9);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("车牌号"), QStringLiteral("能源类型"), QStringLiteral("车型"),
        QStringLiteral("车型等级"), QStringLiteral("类型"), QStringLiteral("载重"),
        QStringLiteral("司机姓名"), QStringLiteral("备注"), QStringLiteral("创建日期")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    root->addLayout(top);
    root->addWidget(m_table, 1);
}

void VehiclePage::onQuery()
{
    const QVector<Vehicle> list = Database::instance().searchVehicles(m_search->text());
    m_table->setRowCount(list.size());
    for (int i = 0; i < list.size(); ++i) {
        const Vehicle& v = list[i];
        const QStringList cells = {
            v.plateNo, v.energyType, v.vehicleType, v.vehicleLevel, v.kind,
            QString::number(v.load), v.driverName, v.remark, v.createdAt};
        for (int col = 0; col < cells.size(); ++col) {
            auto* item = new QTableWidgetItem(cells.at(col));
            item->setData(Qt::UserRole, v.id);
            m_table->setItem(i, col, item);
        }
    }
}

int selectedVehicleId(QTableWidget* table)
{
    const int row = table->currentRow();
    if (row < 0) return -1;
    QTableWidgetItem* item = table->item(row, 0);
    return item ? item->data(Qt::UserRole).toInt() : -1;
}

void VehiclePage::onAdd()
{
    openEditor(-1);
}

void VehiclePage::onEdit()
{
    const int id = selectedVehicleId(m_table);
    if (id < 0) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择一辆车"));
        return;
    }
    openEditor(id);
}

void VehiclePage::openEditor(int id)
{
    QDialog dialog(this);
    dialog.setWindowTitle(id < 0 ? QStringLiteral("新增车辆") : QStringLiteral("编辑车辆"));
    auto* form = new QFormLayout(&dialog);

    auto* plate = new QLineEdit;
    auto* energy = new QComboBox;
    energy->addItems({QStringLiteral("燃油"), QStringLiteral("新能源")});
    auto* type = new QComboBox;
    type->addItems({QStringLiteral("货车"), QStringLiteral("客车")});
    auto* level = new QComboBox;
    level->addItems({QStringLiteral("轿式货车（4M以下）"), QStringLiteral("小型客车（9人及以下）"),
                     QStringLiteral("大型货车"), QStringLiteral("大型客车")});
    auto* kind = new QComboBox;
    kind->addItems({QStringLiteral("常用车"), QStringLiteral("临时车")});
    auto* loadSpin = new QDoubleSpinBox;
    loadSpin->setRange(0, 1000);
    auto* remark = new QLineEdit;

    form->addRow(QStringLiteral("车牌号"), plate);
    form->addRow(QStringLiteral("能源类型"), energy);
    form->addRow(QStringLiteral("车型"), type);
    form->addRow(QStringLiteral("车型等级"), level);
    form->addRow(QStringLiteral("类型"), kind);
    form->addRow(QStringLiteral("载重（吨）"), loadSpin);
    form->addRow(QStringLiteral("备注"), remark);

    Vehicle existing;
    if (id >= 0) {
        const QVector<Vehicle> all = Database::instance().allVehicles();
        for (const Vehicle& v : all) {
            if (v.id == id) { existing = v; break; }
        }
        plate->setText(existing.plateNo);
        energy->setCurrentText(existing.energyType);
        type->setCurrentText(existing.vehicleType);
        level->setCurrentText(existing.vehicleLevel);
        kind->setCurrentText(existing.kind);
        loadSpin->setValue(existing.load);
        remark->setText(existing.remark);
    }

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form->addRow(buttons);

    if (dialog.exec() != QDialog::Accepted) return;
    if (plate->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("车牌号不能为空"));
        return;
    }

    Vehicle v = existing;
    v.plateNo = plate->text().trimmed();
    v.energyType = energy->currentText();
    v.vehicleType = type->currentText();
    v.vehicleLevel = level->currentText();
    v.kind = kind->currentText();
    v.load = loadSpin->value();
    v.remark = remark->text().trimmed();
    if (id < 0) {
        v.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);
        v.driverId = -1;
        Database::instance().createVehicle(v);
        Database::instance().log(Session::instance().currentAccount().username,
                                 QStringLiteral("新增车辆"), v.plateNo);
    } else {
        Database::instance().updateVehicle(v);
        Database::instance().log(Session::instance().currentAccount().username,
                                 QStringLiteral("编辑车辆"), v.plateNo);
    }
    onQuery();
}

void VehiclePage::onRemove()
{
    const int id = selectedVehicleId(m_table);
    if (id < 0) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择一辆车"));
        return;
    }
    const auto ret = QMessageBox::question(this, QStringLiteral("确认"),
        QStringLiteral("确定删除该车辆吗？"), QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;
    Database::instance().deleteVehicle(id);
    Database::instance().log(Session::instance().currentAccount().username,
                             QStringLiteral("删除车辆"), QString::number(id));
    onQuery();
}

void VehiclePage::onAssociateDriver()
{
    const int id = selectedVehicleId(m_table);
    if (id < 0) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择一辆车"));
        return;
    }
    const QVector<Driver> drivers = Database::instance().allDrivers();
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("关联司机"));
    auto* layout = new QVBoxLayout(&dialog);
    auto* combo = new QComboBox;
    combo->addItem(QStringLiteral("（无）"), -1);
    for (const Driver& d : drivers) {
        combo->addItem(d.name, d.id);
    }
    layout->addWidget(new QLabel(QStringLiteral("选择司机")));
    layout->addWidget(combo);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted) return;

    const QVector<Vehicle> all = Database::instance().allVehicles();
    for (const Vehicle& v : all) {
        if (v.id == id) {
            Vehicle updated = v;
            updated.driverId = combo->currentData().toInt();
            Database::instance().updateVehicle(updated);
            Database::instance().log(Session::instance().currentAccount().username,
                                     QStringLiteral("关联司机"), v.plateNo);
            break;
        }
    }
    onQuery();
}

void VehiclePage::onEnter()
{
    onQuery();
}

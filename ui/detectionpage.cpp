#include "detectionpage.h"
#include "core/database.h"
#include "core/session.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QFrame>
#include <QDateTime>
#include <QMessageBox>
#include <QRandomGenerator>

DetectionPage::DetectionPage(QWidget* parent)
    : IPage(parent)
{
    setupUi();
}

void DetectionPage::setupUi()
{
    auto* root = new QVBoxLayout(this);

    // 顶部：模式 + 同时检测车辆数
    auto* top = new QHBoxLayout;
    m_modeCombo = new QComboBox;
    m_modeCombo->addItem(QStringLiteral("单车模式"));
    m_modeCombo->addItem(QStringLiteral("多车模式"));
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DetectionPage::onModeChanged);

    m_countSpin = new QSpinBox;
    m_countSpin->setRange(1, 20);
    m_countSpin->setValue(Database::instance().setting(QStringLiteral("reinit_interval"), QStringLiteral("8")).toInt());

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("检测模式"), m_modeCombo);
    form->addRow(QStringLiteral("同时检测车辆数"), m_countSpin);

    auto* simulateBtn = new QPushButton(QStringLiteral("模拟检测"));
    connect(simulateBtn, &QPushButton::clicked, this, &DetectionPage::onSimulate);

    top->addLayout(form);
    top->addWidget(simulateBtn);
    top->addStretch();

    // 中部：设备显示面板
    m_devicePanel = new QWidget;
    auto* panelLayout = new QVBoxLayout(m_devicePanel);
    panelLayout->setContentsMargins(10, 10, 10, 10);
    panelLayout->setSpacing(6);

    // 底部：当前检测结果
    m_resultTable = new QTableWidget(0, 8);
    m_resultTable->setHorizontalHeaderLabels({
        QStringLiteral("检测编号"), QStringLiteral("探测仪序号"), QStringLiteral("检测结果"),
        QStringLiteral("车牌号"), QStringLiteral("司机"), QStringLiteral("温度"),
        QStringLiteral("湿度"), QStringLiteral("轴数")});
    m_resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto* title = new QLabel(QStringLiteral("检测模式"));
    title->setStyleSheet(QStringLiteral("font-size:18px;font-weight:bold;color:#2c3e50;padding:6px 0;"));

    auto* resultTitle = new QLabel(QStringLiteral("▍当前检测结果"));
    resultTitle->setStyleSheet(QStringLiteral("font-size:14px;font-weight:bold;color:#2c6bbf;padding:6px 0;"));

    root->addWidget(title);
    root->addLayout(top);
    root->addWidget(m_devicePanel, 1);
    root->addWidget(resultTitle);
    root->addWidget(m_resultTable, 2);

    rebuildDevicePanel();
}

void DetectionPage::rebuildDevicePanel()
{
    auto* layout = qobject_cast<QVBoxLayout*>(m_devicePanel->layout());
    Q_ASSERT(layout);
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    const bool showNewParams = Database::instance().setting(QStringLiteral("detect_new_params"), QStringLiteral("true")) == QStringLiteral("true");

    // 手持车辆探测仪
    auto* detector = new QLabel(QStringLiteral("● 手持车辆探测仪   在线"));
    detector->setStyleSheet(QStringLiteral(
        "background:#ecf5ff;border:1px solid #b3d8ff;border-left:4px solid #409eff;"
        "padding:14px;border-radius:6px;color:#2c6bbf;font-weight:bold;font-size:14px;"));
    layout->addWidget(detector);

    if (showNewParams) {
        auto* params = new QLabel(QStringLiteral("    新参数展示：温度 / 湿度 / 速度 / 轴数"));
        params->setStyleSheet(QStringLiteral("color:#606266;padding:6px 0 6px 18px;"));
        layout->addWidget(params);
    }

    // 批注1：有地面传感器才显示分隔线 + 地面传感器信息
    const QVector<Device> devices = Database::instance().allDevices();
    bool hasSensor = false;
    for (const Device& d : devices) {
        if (d.type.contains(QStringLiteral("地面传感器"))) {
            hasSensor = true;
            break;
        }
    }
    if (hasSensor) {
        auto* line = new QFrame;
        line->setFrameShape(QFrame::HLine);
        line->setStyleSheet(QStringLiteral("color:#dcdfe6;"));
        layout->addWidget(line);

        auto* sensor = new QLabel(QStringLiteral("● 地面传感器   在线"));
        sensor->setStyleSheet(QStringLiteral(
            "background:#f0f9eb;border:1px solid #c2e7b0;border-left:4px solid #67c23a;"
            "padding:14px;border-radius:6px;color:#529b2e;font-weight:bold;font-size:14px;"));
        layout->addWidget(sensor);
    }

    layout->addStretch();
}

void DetectionPage::onModeChanged(int index)
{
    m_countSpin->setEnabled(index == 1); // 多车模式才启用同时检测车辆数
}

void DetectionPage::onEnter()
{
    rebuildDevicePanel();
}

void DetectionPage::onSimulate()
{
    const QString plate = QStringLiteral("粤B%1").arg(QRandomGenerator::global()->bounded(10000, 99999));
    Detection d;
    d.no = QStringLiteral("JC") + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmsszzz"));
    d.deviceSerial = QStringLiteral("DT-001");
    d.detector = Session::instance().currentAccount().realName.isEmpty()
                     ? Session::instance().currentAccount().username
                     : Session::instance().currentAccount().realName;
    d.time = QDateTime::currentDateTime();
    d.passed = true;
    d.plateNo = plate;
    d.driverName = QStringLiteral("张三");
    d.vehicleType = QStringLiteral("货车");
    d.category = QStringLiteral("轿式货车（4M以下）");
    d.load = 3.5;
    d.passengers = 2;
    d.reason = QStringLiteral("进场");
    d.temperature = 26.5;
    d.humidity = 58.0;
    d.speed = 5.2;
    d.axleCount = 2;

    if (!Database::instance().createDetection(d)) {
        QMessageBox::warning(this, QStringLiteral("错误"), Database::instance().lastError());
        return;
    }

    const int row = m_resultTable->rowCount();
    m_resultTable->insertRow(row);
    m_resultTable->setItem(row, 0, new QTableWidgetItem(d.no));
    m_resultTable->setItem(row, 1, new QTableWidgetItem(d.deviceSerial));
    m_resultTable->setItem(row, 2, new QTableWidgetItem(QStringLiteral("通过")));
    m_resultTable->setItem(row, 3, new QTableWidgetItem(d.plateNo));
    m_resultTable->setItem(row, 4, new QTableWidgetItem(d.driverName));
    m_resultTable->setItem(row, 5, new QTableWidgetItem(QString::number(d.temperature)));
    m_resultTable->setItem(row, 6, new QTableWidgetItem(QString::number(d.humidity)));
    m_resultTable->setItem(row, 7, new QTableWidgetItem(QString::number(d.axleCount)));

    Database::instance().log(d.detector, QStringLiteral("检测"), QStringLiteral("模拟检测 %1").arg(plate));
}

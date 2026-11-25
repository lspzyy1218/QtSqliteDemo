#include "detectionrecordpage.h"
#include "ui/barchart.h"
#include "core/database.h"
#include "core/session.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QDate>

DetectionRecordPage::DetectionRecordPage(QWidget* parent)
    : IPage(parent)
{
    setupUi();
    onQuery();
}

void DetectionRecordPage::setupUi()
{
    auto* root = new QVBoxLayout(this);

    // 查询条件
    m_plateEdit = new QLineEdit;
    m_plateEdit->setPlaceholderText(QStringLiteral("模糊匹配"));
    m_deviceEdit = new QLineEdit;
    m_deviceEdit->setPlaceholderText(QStringLiteral("请输入"));
    m_resultCombo = new QComboBox;
    m_resultCombo->addItems({QStringLiteral("全部"), QStringLiteral("通过"), QStringLiteral("不通过")});
    m_from = new QDateEdit;
    m_to = new QDateEdit;
    m_from->setCalendarPopup(true);
    m_to->setCalendarPopup(true);
    m_from->setDate(QDate::currentDate().addMonths(-1));
    m_to->setDate(QDate::currentDate());
    m_from->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_to->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));

    auto* filterLayout = new QHBoxLayout;
    filterLayout->addWidget(new QLabel(QStringLiteral("车牌号：")));
    filterLayout->addWidget(m_plateEdit);
    filterLayout->addWidget(new QLabel(QStringLiteral("探测仪序号：")));
    filterLayout->addWidget(m_deviceEdit);
    filterLayout->addWidget(new QLabel(QStringLiteral("检测结果：")));
    filterLayout->addWidget(m_resultCombo);
    filterLayout->addWidget(new QLabel(QStringLiteral("起始：")));
    filterLayout->addWidget(m_from);
    filterLayout->addWidget(new QLabel(QStringLiteral("结束：")));
    filterLayout->addWidget(m_to);

    auto* queryBtn = new QPushButton(QStringLiteral("查询"));
    auto* resetBtn = new QPushButton(QStringLiteral("重置"));
    auto* exportBtn = new QPushButton(QStringLiteral("导出"));
    connect(queryBtn, &QPushButton::clicked, this, &DetectionRecordPage::onQuery);
    connect(resetBtn, &QPushButton::clicked, this, &DetectionRecordPage::onReset);
    connect(exportBtn, &QPushButton::clicked, this, &DetectionRecordPage::onExport);
    if (!Session::instance().has(Permission::DetectionRecord)) {
        exportBtn->setEnabled(false);
    }

    auto* btnLayout = new QHBoxLayout;
    btnLayout->addWidget(queryBtn);
    btnLayout->addWidget(resetBtn);
    btnLayout->addWidget(exportBtn);
    btnLayout->addStretch();

    // 表格
    m_table = new QTableWidget(0, 17);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("检测编号"), QStringLiteral("探测仪序号"), QStringLiteral("检测人"),
        QStringLiteral("检测时间"), QStringLiteral("检测结果"), QStringLiteral("车牌号"),
        QStringLiteral("司机"), QStringLiteral("车型"), QStringLiteral("类别"),
        QStringLiteral("载重"), QStringLiteral("随车人数"), QStringLiteral("进出事由"),
        QStringLiteral("备注"), QStringLiteral("温度"), QStringLiteral("湿度"),
        QStringLiteral("速度"), QStringLiteral("轴数")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_summary = new QLabel;
    m_summary->setStyleSheet(QStringLiteral("font-weight:bold;color:#1a6fb5;"));

    // 统计图表（批注2）
    m_todayChart = new BarChart;
    m_weekChart = new BarChart;
    auto* chartLayout = new QHBoxLayout;
    chartLayout->addWidget(m_todayChart);
    chartLayout->addWidget(m_weekChart);

    root->addLayout(filterLayout);
    root->addLayout(btnLayout);
    root->addWidget(m_table, 2);
    root->addWidget(m_summary);
    root->addLayout(chartLayout, 1);
}

void DetectionRecordPage::onQuery()
{
    const QVector<Detection> list = Database::instance().queryDetections(
        m_from->date(), m_to->date(), m_plateEdit->text(), m_deviceEdit->text(), m_resultCombo->currentText());

    const bool showNewParams = Database::instance().setting(QStringLiteral("detect_new_params"), QStringLiteral("true")) == QStringLiteral("true");
    m_table->setRowCount(list.size());
    for (int i = 0; i < list.size(); ++i) {
        const Detection& d = list[i];
        QStringList cells;
        cells << d.no << d.deviceSerial << d.detector
              << d.time.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"))
              << (d.passed ? QStringLiteral("通过") : QStringLiteral("不通过"))
              << d.plateNo << d.driverName << d.vehicleType << d.category
              << QString::number(d.load) << QString::number(d.passengers)
              << d.reason << d.remark;
        if (showNewParams) {
            cells << QString::number(d.temperature) << QString::number(d.humidity)
                  << QString::number(d.speed) << QString::number(d.axleCount);
        } else {
            cells << QString() << QString() << QString() << QString();
        }
        for (int col = 0; col < cells.size(); ++col) {
            m_table->setItem(i, col, new QTableWidgetItem(cells.at(col)));
        }
    }
    m_summary->setText(QStringLiteral("共 %1 条记录").arg(list.size()));
    refreshStats();
}

void DetectionRecordPage::onReset()
{
    m_plateEdit->clear();
    m_deviceEdit->clear();
    m_resultCombo->setCurrentIndex(0);
    m_from->setDate(QDate::currentDate().addMonths(-1));
    m_to->setDate(QDate::currentDate());
    onQuery();
}

void DetectionRecordPage::onExport()
{
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("导出检测记录"),
        QStringLiteral("检测记录.csv"), QStringLiteral("CSV 文件 (*.csv)"));
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法写入文件"));
        return;
    }
    QTextStream out(&file);
    // 写入 UTF-8 BOM，保证 Excel 正确识别中文
    out << QChar(0xFEFF);
    for (int col = 0; col < m_table->columnCount(); ++col) {
        out << m_table->horizontalHeaderItem(col)->text() << (col == m_table->columnCount() - 1 ? "\n" : ",");
    }
    for (int row = 0; row < m_table->rowCount(); ++row) {
        for (int col = 0; col < m_table->columnCount(); ++col) {
            QTableWidgetItem* item = m_table->item(row, col);
            out << (item ? item->text() : QString()) << (col == m_table->columnCount() - 1 ? "\n" : ",");
        }
    }
    file.close();
    Database::instance().log(Session::instance().currentAccount().username,
                             QStringLiteral("导出"), QStringLiteral("导出检测记录 %1").arg(path));
    QMessageBox::information(this, QStringLiteral("完成"), QStringLiteral("导出成功"));
}

void DetectionRecordPage::refreshStats()
{
    const int total = Database::instance().todayTotal();
    const int passed = Database::instance().todayPassed();
    const int failed = Database::instance().todayFailed();

    auto* todayBar = qobject_cast<BarChart*>(m_todayChart);
    todayBar->setTitle(QStringLiteral("今日检测数：%1 次  通过：%2  异常：%3").arg(total).arg(passed).arg(failed));
    todayBar->setData({QStringLiteral("通过"), QStringLiteral("不通过")}, {passed, failed});

    const QVector<int> counts = Database::instance().lastSevenDaysCounts();
    QVector<QString> labels;
    for (int i = 6; i >= 0; --i) {
        labels.append(QDate::currentDate().addDays(-i).toString(QStringLiteral("M/d")));
    }
    auto* weekBar = qobject_cast<BarChart*>(m_weekChart);
    weekBar->setTitle(QStringLiteral("最近七日检测数量统计"));
    weekBar->setData(labels, counts);
}

void DetectionRecordPage::onEnter()
{
    onQuery();
}

#include "logpage.h"
#include "core/database.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QStringList>

LogPage::LogPage(QWidget* parent)
    : IPage(parent)
{
    setupUi();
    refresh();
}

void LogPage::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->addWidget(new QLabel(QStringLiteral("最近 500 条操作记录")));
    m_table = new QTableWidget(0, 4);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("时间"), QStringLiteral("用户"), QStringLiteral("操作"), QStringLiteral("详情")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    root->addWidget(m_table, 1);
}

void LogPage::refresh()
{
    const QVector<OperationLog> logs = Database::instance().queryLogs(500);
    m_table->setRowCount(logs.size());
    for (int i = 0; i < logs.size(); ++i) {
        const OperationLog& l = logs[i];
        const QStringList cells = {
            l.time.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")), l.user, l.action, l.detail};
        for (int col = 0; col < cells.size(); ++col) {
            m_table->setItem(i, col, new QTableWidgetItem(cells.at(col)));
        }
    }
}

void LogPage::onEnter() { refresh(); }

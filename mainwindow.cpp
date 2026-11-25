#include "mainwindow.h"
#include "database.h"

#include <QAbstractItemView>
#include <QDir>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    // SQLite 文件放在当前用户的AppDataLocation， Windows 上通常为
    // C:/Users/<用户名>/AppData/Local/<组织名>/<应用名>
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    const QString dbPath = dataDir + QStringLiteral("/contacts.db");

    if (!Database::instance().open(dbPath)) {
        QMessageBox::critical(this, QStringLiteral("错误"),
                              QStringLiteral("数据库打开失败：%1").arg(Database::instance().lastError()));
    }

    setupUi();
    refreshTable();

    setWindowTitle(QStringLiteral("联系人管理 - Qt 6 + SQLite"));
    resize(760, 480);
}

void MainWindow::setupUi()
{
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QHBoxLayout(central);

    // 左侧：搜索框 + 联系人表格
    auto* left = new QVBoxLayout;
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(QStringLiteral("搜索姓名 / 电话 / 邮箱…"));
    connect(m_searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearch);

    m_table = new QTableWidget(0, 4);
    m_table->setHorizontalHeaderLabels({QStringLiteral("ID"), QStringLiteral("姓名"),
                                        QStringLiteral("电话"), QStringLiteral("邮箱")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    connect(m_table, &QTableWidget::cellClicked, this, &MainWindow::onRowClicked);

    left->addWidget(m_searchEdit);
    left->addWidget(m_table);

    // 右侧：编辑表单 + 操作按钮
    auto* right = new QVBoxLayout;
    auto* form = new QFormLayout;
    m_nameEdit = new QLineEdit;
    m_phoneEdit = new QLineEdit;
    m_emailEdit = new QLineEdit;
    form->addRow(QStringLiteral("姓名"), m_nameEdit);
    form->addRow(QStringLiteral("电话"), m_phoneEdit);
    form->addRow(QStringLiteral("邮箱"), m_emailEdit);

    m_addBtn = new QPushButton(QStringLiteral("新增"));
    m_updateBtn = new QPushButton(QStringLiteral("修改"));
    m_removeBtn = new QPushButton(QStringLiteral("删除"));
    connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::onAdd);
    connect(m_updateBtn, &QPushButton::clicked, this, &MainWindow::onUpdate);
    connect(m_removeBtn, &QPushButton::clicked, this, &MainWindow::onRemove);

    auto* buttons = new QHBoxLayout;
    buttons->addWidget(m_addBtn);
    buttons->addWidget(m_updateBtn);
    buttons->addWidget(m_removeBtn);

    m_status = new QLabel(QStringLiteral("就绪"));

    right->addLayout(form);
    right->addLayout(buttons);
    right->addWidget(m_status);
    right->addStretch();

    root->addLayout(left, 3);
    root->addLayout(right, 2);
}

int MainWindow::selectedId() const
{
    const int row = m_table->currentRow();
    if (row < 0 || row >= m_table->rowCount()) {
        return -1;
    }
    QTableWidgetItem* item = m_table->item(row, 0);
    return item ? item->data(Qt::UserRole).toInt() : -1;
}

void MainWindow::refreshTable(const QString& keyword)
{
    const QVector<Contact> contacts = keyword.trimmed().isEmpty()
                                          ? Database::instance().allContacts()
                                          : Database::instance().searchContacts(keyword);

    m_table->setRowCount(contacts.size());
    for (int i = 0; i < contacts.size(); ++i) {
        const Contact& c = contacts[i];
        auto* idItem = new QTableWidgetItem(QString::number(c.id));
        idItem->setData(Qt::UserRole, c.id);
        m_table->setItem(i, 0, idItem);
        m_table->setItem(i, 1, new QTableWidgetItem(c.name));
        m_table->setItem(i, 2, new QTableWidgetItem(c.phone));
        m_table->setItem(i, 3, new QTableWidgetItem(c.email));
    }
    m_status->setText(QStringLiteral("共 %1 条记录").arg(contacts.size()));
}

void MainWindow::clearForm()
{
    m_nameEdit->clear();
    m_phoneEdit->clear();
    m_emailEdit->clear();
    m_table->clearSelection();
}

void MainWindow::onRowClicked(int row, int)
{
    if (row < 0 || row >= m_table->rowCount()) {
        return;
    }
    auto textOf = [this, row](int col) -> QString {
        QTableWidgetItem* item = m_table->item(row, col);
        return item ? item->text() : QString();
    };
    m_nameEdit->setText(textOf(1));
    m_phoneEdit->setText(textOf(2));
    m_emailEdit->setText(textOf(3));
}

void MainWindow::onSearch(const QString& text)
{
    refreshTable(text);
}

void MainWindow::onAdd()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("姓名不能为空"));
        return;
    }
    if (Database::instance().addContact(m_nameEdit->text().trimmed(),
                                        m_phoneEdit->text().trimmed(),
                                        m_emailEdit->text().trimmed())) {
        clearForm();
        refreshTable(m_searchEdit->text());
        m_status->setText(QStringLiteral("新增成功"));
    } else {
        QMessageBox::warning(this, QStringLiteral("错误"), Database::instance().lastError());
    }
}

void MainWindow::onUpdate()
{
    const int id = selectedId();
    if (id < 0) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先在左侧表格选择一条记录"));
        return;
    }
    if (Database::instance().updateContact(id,
                                           m_nameEdit->text().trimmed(),
                                           m_phoneEdit->text().trimmed(),
                                           m_emailEdit->text().trimmed())) {
        refreshTable(m_searchEdit->text());
        m_status->setText(QStringLiteral("修改成功"));
    } else {
        QMessageBox::warning(this, QStringLiteral("错误"), Database::instance().lastError());
    }
}

void MainWindow::onRemove()
{
    const int id = selectedId();
    if (id < 0) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先在左侧表格选择一条记录"));
        return;
    }
    const auto answer = QMessageBox::question(
        this, QStringLiteral("确认删除"),
        QStringLiteral("确定要删除 ID 为 %1 的联系人吗？").arg(id),
        QMessageBox::Yes | QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }
    if (Database::instance().removeContact(id)) {
        clearForm();
        refreshTable(m_searchEdit->text());
        m_status->setText(QStringLiteral("删除成功"));
    } else {
        QMessageBox::warning(this, QStringLiteral("错误"), Database::instance().lastError());
    }
}

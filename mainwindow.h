#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QLineEdit;
class QLabel;
class QPushButton;
class QTableWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onAdd();
    void onUpdate();
    void onRemove();
    void onSearch(const QString& text);
    void onRowClicked(int row, int column);

private:
    void setupUi();
    void refreshTable(const QString& keyword = QString());
    void clearForm();
    int  selectedId() const;

    QTableWidget* m_table = nullptr;
    QLineEdit*    m_nameEdit = nullptr;
    QLineEdit*    m_phoneEdit = nullptr;
    QLineEdit*    m_emailEdit = nullptr;
    QLineEdit*    m_searchEdit = nullptr;
    QPushButton*  m_addBtn = nullptr;
    QPushButton*  m_updateBtn = nullptr;
    QPushButton*  m_removeBtn = nullptr;
    QLabel*       m_status = nullptr;
};

#endif // MAINWINDOW_H

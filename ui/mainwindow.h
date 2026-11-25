#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QListWidget;
class QStackedWidget;
class QLabel;
class IPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onMenuClicked(int row);
    void onLogout();
    void refreshMenu();

private:
    void setupUi();
    void addPage(IPage* page);
    void showLicenseExpiredDialog();

    QListWidget* m_menu = nullptr;
    QStackedWidget* m_stack = nullptr;
    QLabel* m_userLabel = nullptr;
    QLabel* m_licenseLabel = nullptr;
};

#endif // MAINWINDOW_H

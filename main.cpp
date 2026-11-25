#include "ui/loginwidget.h"
#include "ui/mainwindow.h"
#include "core/database.h"

#include <QApplication>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QMessageBox>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Example"));
    QCoreApplication::setApplicationName(QStringLiteral("QtSqliteDemo"));

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    const QString dbPath = dataDir + QStringLiteral("/hmi_system.db");

    if (!Database::instance().open(dbPath)) {
        QMessageBox::critical(nullptr, QStringLiteral("错误"),
                              QStringLiteral("数据库打开失败：%1").arg(Database::instance().lastError()));
        return 1;
    }

    LoginWidget login;
    MainWindow mainWindow;

    QObject::connect(&login, &LoginWidget::loginSucceeded, [&]() {
        login.hide();
        mainWindow.show();
    });

    login.show();
    return app.exec();
}

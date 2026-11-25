#include "mainwindow.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Example"));
    QCoreApplication::setApplicationName(QStringLiteral("QtSqliteDemo"));

    MainWindow window;
    window.show();

    return app.exec();
}

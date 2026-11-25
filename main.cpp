#include "ui/loginwidget.h"
#include "ui/mainwindow.h"
#include "core/database.h"

#include <QApplication>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QMessageLogContext>

static QFile g_logFile;
static QMutex g_logMutex;

static void logLine(const QString& msg)
{
    QMutexLocker locker(&g_logMutex);
    if (g_logFile.isOpen()) {
        QTextStream out(&g_logFile);
        out << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"))
            << QStringLiteral(" | ") << msg << '\n';
        g_logFile.flush();
    }
}

static void qtMessageHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg)
{
    Q_UNUSED(ctx)
    QString level;
    switch (type) {
    case QtDebugMsg:    level = QStringLiteral("DEBUG"); break;
    case QtInfoMsg:     level = QStringLiteral("INFO"); break;
    case QtWarningMsg:  level = QStringLiteral("WARN"); break;
    case QtCriticalMsg: level = QStringLiteral("CRITICAL"); break;
    case QtFatalMsg:    level = QStringLiteral("FATAL"); break;
    }
    logLine(level + QStringLiteral(": ") + msg);
}

int main(int argc, char* argv[])
{
    const QString logPath = QCoreApplication::applicationDirPath() + QStringLiteral("/startup.log");
    g_logFile.setFileName(logPath);
    if (g_logFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qInstallMessageHandler(qtMessageHandler);
        logLine(QStringLiteral("===== 程序启动 ====="));
        logLine(QStringLiteral("日志文件: ") + logPath);
    }

    logLine(QStringLiteral("step 1: 创建 QApplication"));
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Example"));
    QCoreApplication::setApplicationName(QStringLiteral("QtSqliteDemo"));
    logLine(QStringLiteral("step 2: 应用信息设置完成"));

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    logLine(QStringLiteral("step 3: 数据目录 = ") + dataDir);
    if (!QDir().mkpath(dataDir)) {
        logLine(QStringLiteral("警告: 创建数据目录失败"));
    }
    const QString dbPath = dataDir + QStringLiteral("/hmi_system.db");
    logLine(QStringLiteral("step 4: 数据库路径 = ") + dbPath);

    if (!Database::instance().open(dbPath)) {
        const QString err = Database::instance().lastError();
        logLine(QStringLiteral("致命: 数据库打开失败 - ") + err);
        QMessageBox::critical(nullptr, QStringLiteral("错误"),
                              QStringLiteral("数据库打开失败：%1").arg(err));
        return 1;
    }
    logLine(QStringLiteral("step 5: 数据库初始化成功"));

    LoginWidget login;
    logLine(QStringLiteral("step 6: 登录窗口构造完成"));

    MainWindow mainWindow;
    logLine(QStringLiteral("step 7: 主窗口构造完成"));

    QObject::connect(&login, &LoginWidget::loginSucceeded, [&]() {
        logLine(QStringLiteral("登录成功，显示主窗口"));
        login.hide();
        mainWindow.show();
    });

    login.show();
    logLine(QStringLiteral("step 8: 登录窗口已显示，进入事件循环"));
    const int result = app.exec();
    logLine(QStringLiteral("程序退出，code=") + QString::number(result));
    return result;
}

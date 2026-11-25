#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QString>
#include <QVector>

struct Contact {
    int     id = -1;
    QString name;
    QString phone;
    QString email;
};

// 数据库访问层：封装 SQLite 连接与联系人表的 CRUD 操作（单例）
class Database
{
public:
    static Database& instance();

    bool open(const QString& dbPath);
    void close();
    bool isOpen() const;
    QString lastError() const;

    bool addContact(const QString& name, const QString& phone, const QString& email);
    bool updateContact(int id, const QString& name, const QString& phone, const QString& email);
    bool removeContact(int id);

    QVector<Contact> allContacts() const;
    QVector<Contact> searchContacts(const QString& keyword) const;

private:
    Database() = default;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool createTable();
    QVector<Contact> queryContacts(const QString& keyword) const;

    QSqlDatabase m_db;
    mutable QString m_error;
};

#endif // DATABASE_H

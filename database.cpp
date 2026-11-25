#include "database.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QtDebug>

Database& Database::instance()
{
    static Database db;
    return db;
}

bool Database::open(const QString& dbPath)
{
    if (m_db.isOpen()) {
        return true;
    }

    // 使用命名连接，避免与其他 QSqlDatabase 实例冲突
    m_db = QSqlDatabase::addDatabase("QSQLITE", "contacts_connection");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        m_error = m_db.lastError().text();
        qWarning() << "[Database] open failed:" << m_error;
        return false;
    }

    return createTable();
}

void Database::close()
{
    const QString connection = m_db.connectionName();
    if (m_db.isOpen()) {
        m_db.close();
    }
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
}

bool Database::isOpen() const
{
    return m_db.isOpen();
}

QString Database::lastError() const
{
    return m_error;
}

bool Database::createTable()
{
    QSqlQuery query(m_db);
    const bool ok = query.exec(
        "CREATE TABLE IF NOT EXISTS contacts ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "phone TEXT, "
        "email TEXT"
        ")");

    if (!ok) {
        m_error = query.lastError().text();
        qWarning() << "[Database] createTable failed:" << m_error;
    }
    return ok;
}

bool Database::addContact(const QString& name, const QString& phone, const QString& email)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO contacts (name, phone, email) VALUES (:name, :phone, :email)");
    query.bindValue(":name", name);
    query.bindValue(":phone", phone);
    query.bindValue(":email", email);

    if (!query.exec()) {
        m_error = query.lastError().text();
        qWarning() << "[Database] insert failed:" << m_error;
        return false;
    }
    return true;
}

bool Database::updateContact(int id, const QString& name, const QString& phone, const QString& email)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE contacts SET name = :name, phone = :phone, email = :email WHERE id = :id");
    query.bindValue(":id", id);
    query.bindValue(":name", name);
    query.bindValue(":phone", phone);
    query.bindValue(":email", email);

    if (!query.exec()) {
        m_error = query.lastError().text();
        qWarning() << "[Database] update failed:" << m_error;
        return false;
    }
    return true;
}

bool Database::removeContact(int id)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM contacts WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        m_error = query.lastError().text();
        qWarning() << "[Database] delete failed:" << m_error;
        return false;
    }
    return true;
}

QVector<Contact> Database::allContacts() const
{
    return queryContacts(QString());
}

QVector<Contact> Database::searchContacts(const QString& keyword) const
{
    return queryContacts(keyword);
}

QVector<Contact> Database::queryContacts(const QString& keyword) const
{
    QVector<Contact> result;
    QSqlQuery query(m_db);

    if (keyword.trimmed().isEmpty()) {
        query.prepare("SELECT id, name, phone, email FROM contacts ORDER BY id");
    } else {
        query.prepare("SELECT id, name, phone, email FROM contacts "
                      "WHERE name LIKE :kw OR phone LIKE :kw OR email LIKE :kw ORDER BY id");
        query.bindValue(":kw", "%" + keyword.trimmed() + "%");
    }

    if (!query.exec()) {
        m_error = query.lastError().text();
        qWarning() << "[Database] select failed:" << m_error;
        return result;
    }

    while (query.next()) {
        Contact c;
        c.id    = query.value(0).toInt();
        c.name  = query.value(1).toString();
        c.phone = query.value(2).toString();
        c.email = query.value(3).toString();
        result.append(c);
    }
    return result;
}

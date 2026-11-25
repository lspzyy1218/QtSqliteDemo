#include "crypto.h"

#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QByteArray>

namespace {
QString toHex(const QByteArray& bytes)
{
    return QString::fromLatin1(bytes.toHex());
}
}

QString Crypto::passwordHash(const QString& password, const QString& salt)
{
    const QByteArray data = (salt + QStringLiteral("|") + password).toUtf8();
    return toHex(QCryptographicHash::hash(data, QCryptographicHash::Sha256));
}

QString Crypto::makeSalt()
{
    QByteArray bytes(16, Qt::Uninitialized);
    for (int i = 0; i < bytes.size(); ++i) {
        bytes[i] = static_cast<char>(QRandomGenerator::global()->bounded(0, 256));
    }
    return toHex(bytes);
}

QString Crypto::licenseKey()
{
    return QStringLiteral("JT-Xin-PeTing-2026-License-Key");
}

QString Crypto::encrypt(const QString& plain, const QString& key)
{
    const QByteArray data = plain.toUtf8();
    const QByteArray k = key.toUtf8();
    QByteArray out(data.size(), Qt::Uninitialized);
    for (int i = 0; i < data.size(); ++i) {
        out[i] = static_cast<char>(data.at(i) ^ k.at(i % k.size()));
    }
    return QString::fromLatin1(out.toBase64());
}

QString Crypto::decrypt(const QString& cipher, const QString& key)
{
    const QByteArray data = QByteArray::fromBase64(cipher.toLatin1());
    const QByteArray k = key.toUtf8();
    QByteArray out(data.size(), Qt::Uninitialized);
    for (int i = 0; i < data.size(); ++i) {
        out[i] = static_cast<char>(data.at(i) ^ k.at(i % k.size()));
    }
    return QString::fromUtf8(out);
}

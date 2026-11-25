#ifndef CRYPTO_H
#define CRYPTO_H

#include <QString>

// 简单安全的密码哈希（SHA-256 + 盐）与授权信息加密（异或 + Base64）
// 注意：授权文件为简单混淆，防止轻易破解修改；非军事级加密。
class Crypto
{
public:
    static QString passwordHash(const QString& password, const QString& salt);
    static QString makeSalt();

    // 授权信息密文（可逆）
    static QString encrypt(const QString& plain, const QString& key);
    static QString decrypt(const QString& cipher, const QString& key);

    static QString licenseKey(); // 内置密钥（实际应用应从硬件指纹派生）
};

#endif // CRYPTO_H

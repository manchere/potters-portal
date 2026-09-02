#include "PasswordAuth.h"

#include <QCryptographicHash>
#include <QPasswordDigestor>
#include <QRandomGenerator>

namespace
{
    constexpr int kPbkdf2Iterations = 100000;
    constexpr int kDerivedKeyLength = 32;
    constexpr int kSaltBytes = 16;
    constexpr int kTokenBytes = 32;

    QString randomHex(int byteCount)
    {
        QByteArray bytes(byteCount, Qt::Uninitialized);
        for (int i = 0; i < byteCount; ++i) {
            bytes[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
        }
        return QString::fromLatin1(bytes.toHex());
    }
}

namespace PasswordAuth
{
    QString generateSalt()
    {
        return randomHex(kSaltBytes);
    }

    QString hashPassword(const QString &password, const QString &salt)
    {
        const QByteArray derived = QPasswordDigestor::deriveKeyPbkdf2(
            QCryptographicHash::Sha256, password.toUtf8(), salt.toUtf8(), kPbkdf2Iterations, kDerivedKeyLength);
        return QString::fromLatin1(derived.toHex());
    }

    bool verifyPassword(const QString &password, const QString &salt, const QString &hash)
    {
        return hashPassword(password, salt) == hash;
    }

    QString generateToken()
    {
        return randomHex(kTokenBytes);
    }

    QString hashToken(const QString &token)
    {
        return QString::fromLatin1(QCryptographicHash::hash(token.toUtf8(), QCryptographicHash::Sha256).toHex());
    }
}

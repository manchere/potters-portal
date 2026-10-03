#include "SessionController.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>

#include "Auth/PasswordAuth.h"
#include "Database/Database.h"

namespace
{
    // Sliding: each authenticated request pushes the expiry this far out
    // again, so a member who opens the app now and then stays signed in.
    constexpr int kSessionLifetimeDays = 180;
}

SessionController::SessionController(QObject *parent)
    : QObject(parent)
{
}

int SessionController::userIdForToken(const QString &token) const
{
    if (token.isEmpty()) {
        return -1;
    }
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "UPDATE sessions SET expires_at = :expires_at "
        "WHERE token_hash = :token_hash AND expires_at > now() RETURNING user_id"));
    query.bindValue(QStringLiteral(":token_hash"), PasswordAuth::hashToken(token));
    query.bindValue(QStringLiteral(":expires_at"), QDateTime::currentDateTimeUtc().addDays(kSessionLifetimeDays));
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return -1;
    }
    return query.value(0).toInt();
}

QString SessionController::createSession(int userId)
{
    Database::ensureConnected();
    const QString token = PasswordAuth::generateToken();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT INTO sessions (user_id, token_hash, expires_at) VALUES (:user_id, :token_hash, :expires_at)"));
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":token_hash"), PasswordAuth::hashToken(token));
    query.bindValue(QStringLiteral(":expires_at"), QDateTime::currentDateTimeUtc().addDays(kSessionLifetimeDays));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return QString();
    }
    return token;
}

bool SessionController::revoke(const QString &token)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM sessions WHERE token_hash = :token_hash"));
    query.bindValue(QStringLiteral(":token_hash"), PasswordAuth::hashToken(token));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

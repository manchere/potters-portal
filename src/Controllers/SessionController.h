#pragma once

#include <QObject>
#include <QString>

// Backed by Postgres (sessions table). Opaque bearer-token sessions used
// by the mobile client only -- the desktop app authenticates in-process
// (see UserController::verifyPassword) and never creates a session row.
class SessionController : public QObject
{
    Q_OBJECT

public:
    explicit SessionController(QObject *parent = nullptr);

    // Returns the id of the user the token belongs to, or -1 if the token
    // is missing/unknown/expired.
    int userIdForToken(const QString &token) const;

    QString lastError() const { return m_lastError; }

public slots:
    // Creates a session for userId valid for 30 days and returns the raw
    // token (only the SHA-256 of it is stored -- see PasswordAuth::hashToken).
    // Returns an empty string on failure.
    QString createSession(int userId);
    bool revoke(const QString &token);

private:
    mutable QString m_lastError;
};

#pragma once

#include <QObject>
#include <QVector>

#include "Models/User.h"

// Backed by Postgres (users table). See ItemController for the query
// pattern. A user account doubles as a Member profile -- see Models/User.h.
class UserController : public QObject
{
    Q_OBJECT

public:
    explicit UserController(QObject *parent = nullptr);

    QVector<User> allUsers() const;
    User userById(int id) const;
    User userByEmail(const QString &email) const;

    // Looks the account up by email and checks the password against its
    // stored PBKDF2 hash (PasswordAuth::verifyPassword). On success,
    // outUser is populated (including passwordHash/passwordSalt, which
    // callers must not forward into any JSON response -- see
    // Json::userToJson). Returns false on either "no such email" or
    // "wrong password", indistinguishably, so callers don't leak which.
    bool verifyPassword(const QString &email, const QString &password, User &outUser) const;

    // Desktop admin unlock is password-only (no email/username field) --
    // tries the password against every Admin account's stored hash and
    // reports the first match. Fine for the small number of Admin accounts
    // a church would realistically have; outUser is whichever Admin the
    // password belonged to, used for attribution (e.g. decided_by).
    bool verifyAdminPassword(const QString &password, User &outUser) const;

    QString lastError() const { return m_lastError; }

public slots:
    // user.passwordHash()/passwordSalt() must already be set (see
    // PasswordAuth) before calling this -- the controller only persists
    // them, it doesn't hash.
    bool addUser(User &user);
    // Updates profile fields and password only -- never is_admin, so a
    // stale User object can't silently grant or revoke Admin. Use
    // setAdminRole() for that.
    bool updateUser(const User &user);
    // Refuses to delete the last remaining Admin.
    bool removeUser(int id);

    // Grants (makeAdmin) or revokes Admin for targetUserId. Only an account
    // that is an Admin *right now* -- re-checked in the database, not
    // trusted from the caller's cached login -- may do this. An Admin can't
    // revoke their own Admin role (another Admin has to), and the last
    // remaining Admin can never be revoked, so the church can't lock itself
    // out of Admin mode. On refusal, lastError() is a user-facing reason.
    bool setAdminRole(int actingUserId, int targetUserId, bool makeAdmin);

signals:
    void usersChanged();

private:
    mutable QString m_lastError;
};

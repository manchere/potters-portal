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
    // `phone` may be in any format; it's normalized before the lookup.
    User userByPhone(const QString &phone) const;

    // Digits only, keeping a leading "+" when given, so "07700 900-123"
    // and "07700900123" are the same number. Every phone stored or looked
    // up goes through this.
    static QString normalizePhone(const QString &phone);
    // At least 7 digits once normalized.
    static bool isValidPhone(const QString &phone);

    // Looks the account up by phone and checks the password against its
    // stored PBKDF2 hash (PasswordAuth::verifyPassword). On success,
    // outUser is populated (including passwordHash/passwordSalt, which
    // callers must not forward into any JSON response -- see
    // Json::userToJson). Returns false on either "no such phone" or
    // "wrong password", indistinguishably, so callers don't leak which.
    bool verifyPassword(const QString &phone, const QString &password, User &outUser) const;

    // Desktop admin unlock is password-only (no phone/username field) --
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
    // phone is normalized before saving, on both add and update.
    // The first Member ever added is made an Admin whatever user.isAdmin()
    // says; user carries the id and Admin flag that were saved.
    bool addUser(User &user);
    // Updates profile fields and password only -- never is_admin, so a
    // stale User object can't silently grant or revoke Admin. Use
    // setAdminRole() for that.
    bool updateUser(const User &user);
    // Hashes newPassword (PasswordAuth) and stores it for userId, leaving
    // every other field alone.
    bool changePassword(int userId, const QString &newPassword);
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

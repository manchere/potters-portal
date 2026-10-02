#include "UserController.h"

#include <QSqlError>
#include <QSqlQuery>

#include "Auth/PasswordAuth.h"
#include "Database/Database.h"

UserController::UserController(QObject *parent)
    : QObject(parent)
{
}

static User userFromQuery(const QSqlQuery &query)
{
    User user;
    user.setId(query.value(QStringLiteral("id")).toInt());
    user.setName(query.value(QStringLiteral("name")).toString());
    user.setEmail(query.value(QStringLiteral("email")).toString());
    user.setPasswordHash(query.value(QStringLiteral("password_hash")).toString());
    user.setPasswordSalt(query.value(QStringLiteral("password_salt")).toString());
    user.setIsAdmin(query.value(QStringLiteral("is_admin")).toBool());
    user.setAvatarSeed(query.value(QStringLiteral("avatar_seed")).toString());
    return user;
}

QVector<User> UserController::allUsers() const
{
    QVector<User> users;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT id, name, email, password_hash, password_salt, is_admin, avatar_seed FROM users ORDER BY name"));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return users;
    }
    while (query.next()) {
        users.append(userFromQuery(query));
    }
    return users;
}

User UserController::userById(int id) const
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT id, name, email, password_hash, password_salt, is_admin, avatar_seed "
        "FROM users WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return User();
    }
    return userFromQuery(query);
}

User UserController::userByEmail(const QString &email) const
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT id, name, email, password_hash, password_salt, is_admin, avatar_seed "
        "FROM users WHERE LOWER(email) = LOWER(:email)"));
    query.bindValue(QStringLiteral(":email"), email);
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return User();
    }
    return userFromQuery(query);
}

bool UserController::verifyPassword(const QString &email, const QString &password, User &outUser) const
{
    const User user = userByEmail(email);
    if (user.id() < 0) {
        return false;
    }
    if (!PasswordAuth::verifyPassword(password, user.passwordSalt(), user.passwordHash())) {
        return false;
    }
    outUser = user;
    return true;
}

bool UserController::verifyAdminPassword(const QString &password, User &outUser) const
{
    for (const User &user : allUsers()) {
        if (user.isAdmin() && PasswordAuth::verifyPassword(password, user.passwordSalt(), user.passwordHash())) {
            outUser = user;
            return true;
        }
    }
    return false;
}

bool UserController::addUser(User &user)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT INTO users (name, email, password_hash, password_salt, is_admin, avatar_seed) "
        "VALUES (:name, :email, :password_hash, :password_salt, :is_admin, :avatar_seed) RETURNING id"));
    query.bindValue(QStringLiteral(":name"), user.name());
    query.bindValue(QStringLiteral(":email"), user.email());
    query.bindValue(QStringLiteral(":password_hash"), user.passwordHash());
    query.bindValue(QStringLiteral(":password_salt"), user.passwordSalt());
    query.bindValue(QStringLiteral(":is_admin"), user.isAdmin());
    query.bindValue(QStringLiteral(":avatar_seed"), user.avatarSeed());
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return false;
    }
    user.setId(query.value(0).toInt());
    emit usersChanged();
    return true;
}

bool UserController::updateUser(const User &user)
{
    // Also writes password_hash/password_salt -- callers that don't want
    // to change the password must carry the existing hash/salt through
    // unchanged (e.g. fetch via userById() first), since this always
    // writes whatever is on the User object.
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "UPDATE users SET name = :name, email = :email, "
        "avatar_seed = :avatar_seed, password_hash = :password_hash, password_salt = :password_salt, "
        "updated_at = now() WHERE id = :id"));
    query.bindValue(QStringLiteral(":name"), user.name());
    query.bindValue(QStringLiteral(":email"), user.email());
    query.bindValue(QStringLiteral(":avatar_seed"), user.avatarSeed());
    query.bindValue(QStringLiteral(":password_hash"), user.passwordHash());
    query.bindValue(QStringLiteral(":password_salt"), user.passwordSalt());
    query.bindValue(QStringLiteral(":id"), user.id());
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    emit usersChanged();
    return true;
}

bool UserController::changePassword(int userId, const QString &newPassword)
{
    const QString salt = PasswordAuth::generateSalt();
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "UPDATE users SET password_hash = :password_hash, password_salt = :password_salt, "
        "updated_at = now() WHERE id = :id"));
    query.bindValue(QStringLiteral(":password_hash"), PasswordAuth::hashPassword(newPassword, salt));
    query.bindValue(QStringLiteral(":password_salt"), salt);
    query.bindValue(QStringLiteral(":id"), userId);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() != 1) {
        m_lastError = QStringLiteral("That account no longer exists.");
        return false;
    }
    emit usersChanged();
    return true;
}

bool UserController::removeUser(int id)
{
    Database::ensureConnected();
    QSqlQuery query;
    // The EXISTS guard keeps the check and the delete atomic:
    // an Admin row is only deleted while at least one other Admin remains.
    query.prepare(QStringLiteral(
        "DELETE FROM users WHERE id = :id "
        "AND (NOT is_admin OR EXISTS (SELECT 1 FROM users other WHERE other.is_admin AND other.id <> :id))"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() == 0) {
        m_lastError = userById(id).id() < 0
            ? QStringLiteral("This member no longer exists.")
            : QStringLiteral("This is the last Admin account and can't be deleted. Make someone else an Admin first.");
        return false;
    }
    emit usersChanged();
    return true;
}

bool UserController::setAdminRole(int actingUserId, int targetUserId, bool makeAdmin)
{
    if (!makeAdmin && actingUserId == targetUserId) {
        m_lastError = QStringLiteral("You can't remove your own Admin role. Ask another Admin to do it.");
        return false;
    }
    const User actingUser = userById(actingUserId);
    if (actingUser.id() < 0 || !actingUser.isAdmin()) {
        m_lastError = QStringLiteral("Only a current Admin can change member roles.");
        return false;
    }

    Database::ensureConnected();
    QSqlQuery query;
    if (makeAdmin) {
        query.prepare(QStringLiteral("UPDATE users SET is_admin = true, updated_at = now() WHERE id = :id"));
    } else {
        query.prepare(QStringLiteral(
            "UPDATE users SET is_admin = false, updated_at = now() WHERE id = :id "
            "AND EXISTS (SELECT 1 FROM users other WHERE other.is_admin AND other.id <> :id)"));
    }
    query.bindValue(QStringLiteral(":id"), targetUserId);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() == 0) {
        m_lastError = userById(targetUserId).id() < 0
            ? QStringLiteral("This member no longer exists.")
            : QStringLiteral("There must always be at least one Admin.");
        return false;
    }
    emit usersChanged();
    return true;
}

#include "RoleTypeController.h"

#include <QSqlError>
#include <QSqlQuery>

#include "Database/Database.h"

RoleTypeController::RoleTypeController(QObject *parent)
    : QObject(parent)
{
}

QVector<RoleType> RoleTypeController::allRoleTypes() const
{
    QVector<RoleType> roleTypes;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT id, name, icon FROM assignment_roles ORDER BY name"));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return roleTypes;
    }
    while (query.next()) {
        roleTypes.append(RoleType(query.value(0).toInt(), query.value(1).toString(), query.value(2).toString()));
    }
    return roleTypes;
}

RoleType RoleTypeController::roleTypeById(int id) const
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT id, name, icon FROM assignment_roles WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return RoleType();
    }
    return RoleType(query.value(0).toInt(), query.value(1).toString(), query.value(2).toString());
}

bool RoleTypeController::addRoleType(RoleType &roleType)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT INTO assignment_roles (name, icon) VALUES (:name, :icon) RETURNING id"));
    query.bindValue(QStringLiteral(":name"), roleType.name());
    query.bindValue(QStringLiteral(":icon"), roleType.icon());
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return false;
    }
    roleType.setId(query.value(0).toInt());
    emit roleTypesChanged();
    return true;
}

bool RoleTypeController::updateRoleType(const RoleType &roleType)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "UPDATE assignment_roles SET name = :name, icon = :icon, updated_at = now() WHERE id = :id"));
    query.bindValue(QStringLiteral(":name"), roleType.name());
    query.bindValue(QStringLiteral(":icon"), roleType.icon());
    query.bindValue(QStringLiteral(":id"), roleType.id());
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    emit roleTypesChanged();
    return true;
}

bool RoleTypeController::removeRoleType(int id)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM assignment_roles WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        // Most likely cause: assignments still reference this role type
        // (assignments.role_id has no ON DELETE action, since the column
        // is NOT NULL and orphaning it would be worse than refusing).
        m_lastError = query.lastError().text();
        return false;
    }
    emit roleTypesChanged();
    return true;
}

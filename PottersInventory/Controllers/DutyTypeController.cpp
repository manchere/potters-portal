#include "DutyTypeController.h"

#include <QSqlError>
#include <QSqlQuery>

#include "Database/Database.h"

DutyTypeController::DutyTypeController(QObject *parent)
    : QObject(parent)
{
}

QVector<DutyType> DutyTypeController::allDutyTypes() const
{
    QVector<DutyType> dutyTypes;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT id, name, icon FROM duty_types ORDER BY name"));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return dutyTypes;
    }
    while (query.next()) {
        dutyTypes.append(DutyType(query.value(0).toInt(), query.value(1).toString(), query.value(2).toString()));
    }
    return dutyTypes;
}

DutyType DutyTypeController::dutyTypeById(int id) const
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT id, name, icon FROM duty_types WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return DutyType();
    }
    return DutyType(query.value(0).toInt(), query.value(1).toString(), query.value(2).toString());
}

bool DutyTypeController::addDutyType(DutyType &dutyType)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT INTO duty_types (name, icon) VALUES (:name, :icon) RETURNING id"));
    query.bindValue(QStringLiteral(":name"), dutyType.name());
    query.bindValue(QStringLiteral(":icon"), dutyType.icon());
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return false;
    }
    dutyType.setId(query.value(0).toInt());
    emit dutyTypesChanged();
    return true;
}

bool DutyTypeController::updateDutyType(const DutyType &dutyType)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "UPDATE duty_types SET name = :name, icon = :icon, updated_at = now() WHERE id = :id"));
    query.bindValue(QStringLiteral(":name"), dutyType.name());
    query.bindValue(QStringLiteral(":icon"), dutyType.icon());
    query.bindValue(QStringLiteral(":id"), dutyType.id());
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    emit dutyTypesChanged();
    return true;
}

bool DutyTypeController::removeDutyType(int id)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM duty_types WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        // Most likely cause: duties still reference this duty type
        // (duties.duty_type_id has no ON DELETE action, since the column
        // is NOT NULL and orphaning it would be worse than refusing).
        m_lastError = query.lastError().text();
        return false;
    }
    emit dutyTypesChanged();
    return true;
}

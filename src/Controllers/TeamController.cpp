#include "TeamController.h"

#include <QSqlError>
#include <QSqlQuery>

#include "Database/Database.h"

namespace
{
    // The unique index on LOWER(name) is the usual reason a save fails.
    QString saveError(const QSqlQuery &query, const QString &name)
    {
        const QString error = query.lastError().text();
        if (error.contains(QStringLiteral("teams_name_key"))) {
            return QStringLiteral("There's already a team called \"%1\".").arg(name);
        }
        return error;
    }
}

TeamController::TeamController(QObject *parent)
    : QObject(parent)
{
}

QVector<Team> TeamController::allTeams() const
{
    QVector<Team> teams;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT id, name, description FROM teams ORDER BY name"));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return teams;
    }
    while (query.next()) {
        teams.append(Team(query.value(0).toInt(), query.value(1).toString(), query.value(2).toString()));
    }
    return teams;
}

Team TeamController::teamById(int id) const
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT id, name, description FROM teams WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return Team();
    }
    return Team(query.value(0).toInt(), query.value(1).toString(), query.value(2).toString());
}

bool TeamController::addTeam(Team &team)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT INTO teams (name, description) VALUES (:name, :description) RETURNING id"));
    query.bindValue(QStringLiteral(":name"), team.name());
    query.bindValue(QStringLiteral(":description"), team.description());
    if (!query.exec() || !query.next()) {
        m_lastError = saveError(query, team.name());
        return false;
    }
    team.setId(query.value(0).toInt());
    emit teamsChanged();
    return true;
}

bool TeamController::updateTeam(const Team &team)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "UPDATE teams SET name = :name, description = :description, updated_at = now() WHERE id = :id"));
    query.bindValue(QStringLiteral(":name"), team.name());
    query.bindValue(QStringLiteral(":description"), team.description());
    query.bindValue(QStringLiteral(":id"), team.id());
    if (!query.exec()) {
        m_lastError = saveError(query, team.name());
        return false;
    }
    emit teamsChanged();
    return true;
}

bool TeamController::removeTeam(int id)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM teams WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    emit teamsChanged();
    return true;
}

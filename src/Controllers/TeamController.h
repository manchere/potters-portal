#pragma once

#include <QObject>
#include <QVector>

#include "Models/Team.h"

// Backed by Postgres (teams table). See DutyTypeController for the query
// pattern. Members join a team through users.team_id (UserController).
class TeamController : public QObject
{
    Q_OBJECT

public:
    explicit TeamController(QObject *parent = nullptr);

    QVector<Team> allTeams() const;
    Team teamById(int id) const;

    QString lastError() const { return m_lastError; }

public slots:
    bool addTeam(Team &team);
    bool updateTeam(const Team &team);
    // Its Members stay, just without a team (ON DELETE SET NULL).
    bool removeTeam(int id);

signals:
    void teamsChanged();

private:
    mutable QString m_lastError;
};

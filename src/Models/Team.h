#pragma once

#include <QString>

// A group of Members (e.g. Choir, Ushers, Media), managed from the desktop
// Taxonomy tab. Each Member belongs to at most one team (User::teamId).
class Team
{
public:
    Team() = default;
    Team(int id, QString name, QString description = QString());

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }

    QString description() const { return m_description; }
    void setDescription(const QString &description) { m_description = description; }

private:
    int m_id = -1;
    QString m_name;
    QString m_description;
};

#include "AccessController.h"

#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "Database/Database.h"

namespace
{
    bool sectionFromKey(const QString &key, Section *section)
    {
        for (Section candidate : allSections()) {
            if (sectionKey(candidate) == key) {
                *section = candidate;
                return true;
            }
        }
        return false;
    }

    SectionAccess accessFromQuery(const QSqlQuery &query, int firstColumn)
    {
        return SectionAccess{query.value(firstColumn).toBool(), query.value(firstColumn + 1).toBool(),
                             query.value(firstColumn + 2).toBool(), query.value(firstColumn + 3).toBool()};
    }
}

AccessController::AccessController(QObject *parent)
    : QObject(parent)
{
}

AccessRights AccessController::rightsFor(const User &user, const QDate &upcomingSunday) const
{
    if (user.id() >= 0 && user.isAdmin()) {
        return AccessRights::full();
    }

    AccessRights rights;
    Database::ensureConnected();
    QSqlQuery query;
    // One query for every rule that applies: everyone's, the member's own,
    // their team's, and each duty they have on the upcoming Sunday.
    query.prepare(QStringLiteral(
        "SELECT section, can_view, can_create, can_update, can_delete FROM access_rules "
        "WHERE subject_kind = 'everyone' "
        "   OR (subject_kind = 'member' AND subject_id = :member_id) "
        "   OR (subject_kind = 'team' AND subject_id = :team_id) "
        "   OR (subject_kind = 'duty' AND subject_id IN ("
        "         SELECT duty_type_id FROM duties WHERE member_id = :member_id AND service_date = :sunday))"));
    query.bindValue(QStringLiteral(":member_id"), user.id() >= 0 ? user.id() : -1);
    query.bindValue(QStringLiteral(":team_id"), user.id() >= 0 && user.teamId() > 0 ? user.teamId() : -1);
    query.bindValue(QStringLiteral(":sunday"), upcomingSunday);
    if (!query.exec()) {
        // Rules unreadable (e.g. the table isn't there yet): let everyone
        // look around, but not change anything.
        m_lastError = query.lastError().text();
        for (Section section : allSections()) {
            rights.grant(section, SectionAccess{true, false, false, false});
        }
        return rights;
    }
    while (query.next()) {
        Section section;
        if (sectionFromKey(query.value(0).toString(), &section)) {
            rights.grant(section, accessFromQuery(query, 1));
        }
    }
    return rights;
}

QHash<Section, SectionAccess> AccessController::rulesFor(AccessSubject subject, int subjectId) const
{
    QHash<Section, SectionAccess> rules;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT section, can_view, can_create, can_update, can_delete FROM access_rules "
        "WHERE subject_kind = :kind AND subject_id = :id"));
    query.bindValue(QStringLiteral(":kind"), subjectKey(subject));
    query.bindValue(QStringLiteral(":id"), subject == AccessSubject::Everyone ? 0 : subjectId);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return rules;
    }
    while (query.next()) {
        Section section;
        if (sectionFromKey(query.value(0).toString(), &section)) {
            rules.insert(section, accessFromQuery(query, 1));
        }
    }
    return rules;
}

QSet<QString> AccessController::subjectsWithRules() const
{
    QSet<QString> subjects;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT DISTINCT subject_kind, subject_id FROM access_rules "
        "WHERE can_view OR can_create OR can_update OR can_delete"));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return subjects;
    }
    while (query.next()) {
        subjects.insert(query.value(0).toString() + QLatin1Char(':') + query.value(1).toString());
    }
    return subjects;
}

bool AccessController::setRule(AccessSubject subject, int subjectId, Section section, const SectionAccess &access)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT INTO access_rules (subject_kind, subject_id, section, can_view, can_create, can_update, can_delete) "
        "VALUES (:kind, :id, :section, :view, :create, :update, :delete) "
        "ON CONFLICT (subject_kind, subject_id, section) DO UPDATE SET "
        "can_view = EXCLUDED.can_view, can_create = EXCLUDED.can_create, "
        "can_update = EXCLUDED.can_update, can_delete = EXCLUDED.can_delete, updated_at = now()"));
    query.bindValue(QStringLiteral(":kind"), subjectKey(subject));
    query.bindValue(QStringLiteral(":id"), subject == AccessSubject::Everyone ? 0 : subjectId);
    query.bindValue(QStringLiteral(":section"), sectionKey(section));
    query.bindValue(QStringLiteral(":view"), access.view);
    query.bindValue(QStringLiteral(":create"), access.create);
    query.bindValue(QStringLiteral(":update"), access.update);
    query.bindValue(QStringLiteral(":delete"), access.remove);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    emit rulesChanged();
    return true;
}

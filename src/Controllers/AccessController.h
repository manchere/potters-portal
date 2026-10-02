#pragma once

#include <QDate>
#include <QHash>
#include <QObject>
#include <QSet>

#include "Models/AccessRights.h"
#include "Models/User.h"

// Backed by Postgres (access_rules table). Reads and saves the rules an
// Admin sets in Settings > Access Rights, and works out what one person
// may do (see rightsFor).
class AccessController : public QObject
{
    Q_OBJECT

public:
    explicit AccessController(QObject *parent = nullptr);

    // What `user` may do: Admins get everything; anyone else gets every
    // section/action allowed by the "everyone" rules, their own member
    // rules, their team's rules, and the rules of each duty they have on
    // `upcomingSunday`, all added together. Pass User() for someone not
    // signed in (only the "everyone" rules apply).
    AccessRights rightsFor(const User &user, const QDate &upcomingSunday) const;

    // The rules stored for one subject, by section (missing = nothing).
    QHash<Section, SectionAccess> rulesFor(AccessSubject subject, int subjectId) const;
    // Subjects (as "kind:id") that have at least one rule granting anything,
    // so the Access Rights list can mark them.
    QSet<QString> subjectsWithRules() const;

    QString lastError() const { return m_lastError; }

public slots:
    // Replaces what `subject` may do in `section`.
    bool setRule(AccessSubject subject, int subjectId, Section section, const SectionAccess &access);

signals:
    void rulesChanged();

private:
    mutable QString m_lastError;
};

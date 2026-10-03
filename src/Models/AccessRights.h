#pragma once

#include <QHash>
#include <QString>
#include <QVector>

// What someone may do on the desktop app, per section (sidebar tab): open
// it, and create / update / delete there. Set by an Admin in Settings >
// Access Rights as rules for everyone, a member, a team, or a duty; see
// database/migrations/0025_create_access_rules.sql and AccessController.
// The Schedule tab isn't a section here: everyone can see it and only
// Admins change it.

enum class Section
{
    Reports,
    Songs,
    Inventory,
    Taxonomy,
    Feedback,
    Settings,
};

inline size_t qHash(Section section, size_t seed = 0)
{
    return ::qHash(static_cast<int>(section), seed);
}

enum class AccessAction
{
    View,
    Create,
    Update,
    Delete,
};

inline size_t qHash(AccessAction action, size_t seed = 0)
{
    return ::qHash(static_cast<int>(action), seed);
}

struct SectionAccess
{
    bool view = false;
    bool create = false;
    bool update = false;
    bool remove = false;

    bool allows(AccessAction action) const;
    void set(AccessAction action, bool allowed);
    SectionAccess &operator|=(const SectionAccess &other);
};

// The sections in sidebar order.
QVector<Section> allSections();
// How a section is stored in access_rules.section ("reports", ...).
QString sectionKey(Section section);
// False for actions a section has nothing for -- e.g. Settings can only
// be viewed and have its theme changed (Update), and Reports can only be
// viewed and saved (Create).
bool sectionHasAction(Section section, AccessAction action);

// Who a rule is for.
enum class AccessSubject
{
    Everyone,
    Member,
    Team,
    Duty,
};
QString subjectKey(AccessSubject subject);

// Everything one person may do, across sections.
class AccessRights
{
public:
    // Every action in every section -- what an Admin gets.
    static AccessRights full();

    SectionAccess section(Section section) const { return m_sections.value(section); }
    void grant(Section section, const SectionAccess &access) { m_sections[section] |= access; }

private:
    QHash<Section, SectionAccess> m_sections;
};

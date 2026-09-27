#pragma once

#include <QString>

// An admin-manageable assignment duty type (SCHEDULING_FUNCTIONAL_REQUIREMENTS.md
// FR-6): name + a single emoji icon shown throughout the scheduling UI.
// Replaces the old fixed AssignmentRole enum (database/migrations/0018,
// 0019) -- Assignment now stores a roleId pointing at one of these instead
// of a hardcoded value, so an Admin can define new duty types from the
// desktop Taxonomy tab.
class RoleType
{
public:
    RoleType() = default;
    RoleType(int id, QString name, QString icon);

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }

    QString icon() const { return m_icon; }
    void setIcon(const QString &icon) { m_icon = icon; }

    // icon() + name() combined, e.g. "🎤  Singing" -- used in combo boxes
    // and list rows.
    QString iconAndName() const;

private:
    int m_id = -1;
    QString m_name;
    QString m_icon = QStringLiteral("\U0001F4CB"); // clipboard, generic default
};

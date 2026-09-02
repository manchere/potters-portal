#pragma once

#include <QDate>
#include <QString>
#include <QVector>

enum class AssignmentRole
{
    Singing,
    Translating,
    Preaching,
    Offering,
    Announcement
};

// Matches the CHECK constraint on assignments.role in
// database/migrations/0017_assignments_role_enum.sql.
QString assignmentRoleToString(AssignmentRole role);
AssignmentRole assignmentRoleFromString(const QString &role);
QVector<AssignmentRole> allAssignmentRoles();

// A Sunday duty assignment (SCHEDULING_FUNCTIONAL_REQUIREMENTS.md FR-6,
// FR-7). supportMemberId is an optional backup covering for memberId if
// the primary assignee isn't present (FR-7.2); -1 means "none set" for
// either id, matching the -1 "unset" convention used by Item/Category.
class Assignment
{
public:
    Assignment() = default;
    Assignment(int id, AssignmentRole role, QDate serviceDate, int memberId, int supportMemberId, QString notes);

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    AssignmentRole role() const { return m_role; }
    void setRole(AssignmentRole role) { m_role = role; }

    QDate serviceDate() const { return m_serviceDate; }
    void setServiceDate(const QDate &serviceDate) { m_serviceDate = serviceDate; }

    int memberId() const { return m_memberId; }
    void setMemberId(int memberId) { m_memberId = memberId; }

    int supportMemberId() const { return m_supportMemberId; }
    void setSupportMemberId(int supportMemberId) { m_supportMemberId = supportMemberId; }

    QString notes() const { return m_notes; }
    void setNotes(const QString &notes) { m_notes = notes; }

private:
    int m_id = -1;
    AssignmentRole m_role = AssignmentRole::Singing;
    QDate m_serviceDate;
    int m_memberId = -1;
    int m_supportMemberId = -1;
    QString m_notes;
};

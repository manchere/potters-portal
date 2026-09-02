#include "Assignment.h"

QString assignmentRoleToString(AssignmentRole role)
{
    switch (role) {
    case AssignmentRole::Singing: return QStringLiteral("singing");
    case AssignmentRole::Translating: return QStringLiteral("translating");
    case AssignmentRole::Preaching: return QStringLiteral("preaching");
    case AssignmentRole::Offering: return QStringLiteral("offering");
    case AssignmentRole::Announcement: return QStringLiteral("announcement");
    }
    return QStringLiteral("singing");
}

AssignmentRole assignmentRoleFromString(const QString &role)
{
    if (role == QStringLiteral("translating")) return AssignmentRole::Translating;
    if (role == QStringLiteral("preaching")) return AssignmentRole::Preaching;
    if (role == QStringLiteral("offering")) return AssignmentRole::Offering;
    if (role == QStringLiteral("announcement")) return AssignmentRole::Announcement;
    return AssignmentRole::Singing;
}

QVector<AssignmentRole> allAssignmentRoles()
{
    return {
        AssignmentRole::Singing,
        AssignmentRole::Translating,
        AssignmentRole::Preaching,
        AssignmentRole::Offering,
        AssignmentRole::Announcement,
    };
}

Assignment::Assignment(int id, AssignmentRole role, QDate serviceDate, int memberId, int supportMemberId, QString notes)
    : m_id(id)
    , m_role(role)
    , m_serviceDate(serviceDate)
    , m_memberId(memberId)
    , m_supportMemberId(supportMemberId)
    , m_notes(std::move(notes))
{
}

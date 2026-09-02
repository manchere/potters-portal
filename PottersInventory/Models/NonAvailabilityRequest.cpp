#include "NonAvailabilityRequest.h"

QString requestStatusToString(RequestStatus status)
{
    switch (status) {
    case RequestStatus::Pending: return QStringLiteral("pending");
    case RequestStatus::Approved: return QStringLiteral("approved");
    case RequestStatus::Denied: return QStringLiteral("denied");
    }
    return QStringLiteral("pending");
}

RequestStatus requestStatusFromString(const QString &status)
{
    if (status == QStringLiteral("approved")) return RequestStatus::Approved;
    if (status == QStringLiteral("denied")) return RequestStatus::Denied;
    return RequestStatus::Pending;
}

NonAvailabilityRequest::NonAvailabilityRequest(int id, int assignmentId, int userId, QString message)
    : m_id(id)
    , m_assignmentId(assignmentId)
    , m_userId(userId)
    , m_message(std::move(message))
{
}

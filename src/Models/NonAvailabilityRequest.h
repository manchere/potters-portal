#pragma once

#include <QDateTime>
#include <QString>

enum class RequestStatus
{
    Pending,
    Approved,
    Denied
};

// Matches the CHECK constraint on non_availability_requests.status in
// database/migrations/0016_create_non_availability_requests.sql.
QString requestStatusToString(RequestStatus status);
RequestStatus requestStatusFromString(const QString &status);

// A formal, duty-specific non-availability request
// (SCHEDULING_FUNCTIONAL_REQUIREMENTS.md FR-4, FR-5): only valid against
// a duty the requesting user is already on, requires a message,
// and is decided (approved/denied) by an Admin.
class NonAvailabilityRequest
{
public:
    NonAvailabilityRequest() = default;
    NonAvailabilityRequest(int id, int dutyId, int userId, QString message);

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    int dutyId() const { return m_dutyId; }
    void setDutyId(int dutyId) { m_dutyId = dutyId; }

    int userId() const { return m_userId; }
    void setUserId(int userId) { m_userId = userId; }

    QString message() const { return m_message; }
    void setMessage(const QString &message) { m_message = message; }

    RequestStatus status() const { return m_status; }
    void setStatus(RequestStatus status) { m_status = status; }

    int decidedBy() const { return m_decidedBy; }
    void setDecidedBy(int decidedBy) { m_decidedBy = decidedBy; }

    QDateTime decidedAt() const { return m_decidedAt; }
    void setDecidedAt(const QDateTime &decidedAt) { m_decidedAt = decidedAt; }

    QDateTime createdAt() const { return m_createdAt; }
    void setCreatedAt(const QDateTime &createdAt) { m_createdAt = createdAt; }

private:
    int m_id = -1;
    int m_dutyId = -1;
    int m_userId = -1;
    QString m_message;
    RequestStatus m_status = RequestStatus::Pending;
    int m_decidedBy = -1;
    QDateTime m_decidedAt;
    QDateTime m_createdAt;
};

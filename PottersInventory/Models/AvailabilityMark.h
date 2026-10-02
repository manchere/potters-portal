#pragma once

#include <QDate>

// A day a Member marked as "expect not to be present" on their general
// calendar (SCHEDULING_FUNCTIONAL_REQUIREMENTS.md FR-3.1/3.2). Purely
// informational -- distinct from NonAvailabilityRequest, which is a
// formal, duty-specific request that goes through Admin approval.
class AvailabilityMark
{
public:
    AvailabilityMark() = default;
    AvailabilityMark(int id, int userId, QDate date);

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    int userId() const { return m_userId; }
    void setUserId(int userId) { m_userId = userId; }

    QDate date() const { return m_date; }
    void setDate(const QDate &date) { m_date = date; }

private:
    int m_id = -1;
    int m_userId = -1;
    QDate m_date;
};

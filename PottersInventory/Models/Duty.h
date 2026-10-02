#pragma once

#include <QDate>
#include <QString>

// A Sunday duty (SCHEDULING_FUNCTIONAL_REQUIREMENTS.md FR-6,
// FR-7). dutyTypeId points at a duty_types row (Models/DutyType.h) --
// an admin-manageable duty type, not a fixed enum. supportMemberId is an
// optional backup covering for memberId if the primary assignee isn't
// present (FR-7.2); -1 means "none set" for either id, matching the -1
// "unset" convention used by Item/Category.
class Duty
{
public:
    Duty() = default;
    Duty(int id, int dutyTypeId, QDate serviceDate, int memberId, int supportMemberId, QString notes);

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    int dutyTypeId() const { return m_dutyTypeId; }
    void setDutyTypeId(int dutyTypeId) { m_dutyTypeId = dutyTypeId; }

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
    int m_dutyTypeId = -1;
    QDate m_serviceDate;
    int m_memberId = -1;
    int m_supportMemberId = -1;
    QString m_notes;
};

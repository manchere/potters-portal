#include "Duty.h"

Duty::Duty(int id, int dutyTypeId, QDate serviceDate, int memberId, int supportMemberId, QString notes)
    : m_id(id)
    , m_dutyTypeId(dutyTypeId)
    , m_serviceDate(serviceDate)
    , m_memberId(memberId)
    , m_supportMemberId(supportMemberId)
    , m_notes(std::move(notes))
{
}

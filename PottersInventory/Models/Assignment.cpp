#include "Assignment.h"

Assignment::Assignment(int id, int roleId, QDate serviceDate, int memberId, int supportMemberId, QString notes)
    : m_id(id)
    , m_roleId(roleId)
    , m_serviceDate(serviceDate)
    , m_memberId(memberId)
    , m_supportMemberId(supportMemberId)
    , m_notes(std::move(notes))
{
}

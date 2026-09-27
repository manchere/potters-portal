#include "RoleType.h"

RoleType::RoleType(int id, QString name, QString icon)
    : m_id(id)
    , m_name(std::move(name))
    , m_icon(std::move(icon))
{
}

QString RoleType::iconAndName() const
{
    return m_icon + QStringLiteral("  ") + m_name;
}

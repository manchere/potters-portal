#include "User.h"

User::User(int id, QString name, QString phone, bool isAdmin, QString color)
    : m_id(id)
    , m_name(std::move(name))
    , m_phone(std::move(phone))
    , m_isAdmin(isAdmin)
    , m_color(std::move(color))
{
}

#include "User.h"

User::User(int id, QString name, QString email, bool isAdmin, QString color)
    : m_id(id)
    , m_name(std::move(name))
    , m_email(std::move(email))
    , m_isAdmin(isAdmin)
    , m_color(std::move(color))
{
}

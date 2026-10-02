#include "Team.h"

Team::Team(int id, QString name, QString description)
    : m_id(id)
    , m_name(std::move(name))
    , m_description(std::move(description))
{
}

#include "Tag.h"

Tag::Tag(int id, QString name, QString color, QString description)
    : m_id(id)
    , m_name(std::move(name))
    , m_color(std::move(color))
    , m_description(std::move(description))
{
}

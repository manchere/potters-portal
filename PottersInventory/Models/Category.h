#pragma once

#include <QString>

class Category
{
public:
    Category() = default;
    Category(int id, QString name, QString description = QString());

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }

    QString description() const { return m_description; }
    void setDescription(const QString &description) { m_description = description; }

private:
    int m_id = -1;
    QString m_name;
    QString m_description;
};

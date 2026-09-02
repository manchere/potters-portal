#pragma once

#include <QString>

class Tag
{
public:
    Tag() = default;
    Tag(int id, QString name, QString color = QStringLiteral("#3b82f6"), QString description = QString());

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }

    QString color() const { return m_color; }
    void setColor(const QString &color) { m_color = color; }

    QString description() const { return m_description; }
    void setDescription(const QString &description) { m_description = description; }

private:
    int m_id = -1;
    QString m_name;
    QString m_color = QStringLiteral("#3b82f6");
    QString m_description;
};

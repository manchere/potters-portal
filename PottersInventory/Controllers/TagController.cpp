#include "TagController.h"

#include <QSqlError>
#include <QSqlQuery>

#include "Database/Database.h"

TagController::TagController(QObject *parent)
    : QObject(parent)
{
}

QVector<Tag> TagController::allTags() const
{
    QVector<Tag> tags;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT id, name, color, description FROM tags ORDER BY name"));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return tags;
    }
    while (query.next()) {
        tags.append(Tag(query.value(0).toInt(), query.value(1).toString(), query.value(2).toString(), query.value(3).toString()));
    }
    return tags;
}

Tag TagController::tagById(int id) const
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT id, name, color, description FROM tags WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return Tag();
    }
    return Tag(query.value(0).toInt(), query.value(1).toString(), query.value(2).toString(), query.value(3).toString());
}

bool TagController::addTag(Tag &tag)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT INTO tags (name, color, description) VALUES (:name, :color, :description) RETURNING id"));
    query.bindValue(QStringLiteral(":name"), tag.name());
    query.bindValue(QStringLiteral(":color"), tag.color());
    query.bindValue(QStringLiteral(":description"), tag.description());
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return false;
    }
    tag.setId(query.value(0).toInt());
    emit tagsChanged();
    return true;
}

bool TagController::updateTag(const Tag &tag)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "UPDATE tags SET name = :name, color = :color, description = :description, updated_at = now() WHERE id = :id"));
    query.bindValue(QStringLiteral(":name"), tag.name());
    query.bindValue(QStringLiteral(":color"), tag.color());
    query.bindValue(QStringLiteral(":description"), tag.description());
    query.bindValue(QStringLiteral(":id"), tag.id());
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    emit tagsChanged();
    return true;
}

bool TagController::removeTag(int id)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM tags WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    emit tagsChanged();
    return true;
}

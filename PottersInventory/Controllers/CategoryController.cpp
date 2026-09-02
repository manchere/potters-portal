#include "CategoryController.h"

#include <QSqlError>
#include <QSqlQuery>

#include "Database/Database.h"

CategoryController::CategoryController(QObject *parent)
    : QObject(parent)
{
}

QVector<Category> CategoryController::allCategories() const
{
    QVector<Category> categories;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT id, name, description FROM categories ORDER BY name"));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return categories;
    }
    while (query.next()) {
        categories.append(Category(query.value(0).toInt(), query.value(1).toString(), query.value(2).toString()));
    }
    return categories;
}

Category CategoryController::categoryById(int id) const
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT id, name, description FROM categories WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return Category();
    }
    return Category(query.value(0).toInt(), query.value(1).toString(), query.value(2).toString());
}

bool CategoryController::addCategory(Category &category)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT INTO categories (name, description) VALUES (:name, :description) RETURNING id"));
    query.bindValue(QStringLiteral(":name"), category.name());
    query.bindValue(QStringLiteral(":description"), category.description());
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return false;
    }
    category.setId(query.value(0).toInt());
    emit categoriesChanged();
    return true;
}

bool CategoryController::updateCategory(const Category &category)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "UPDATE categories SET name = :name, description = :description, updated_at = now() WHERE id = :id"));
    query.bindValue(QStringLiteral(":name"), category.name());
    query.bindValue(QStringLiteral(":description"), category.description());
    query.bindValue(QStringLiteral(":id"), category.id());
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    emit categoriesChanged();
    return true;
}

bool CategoryController::removeCategory(int id)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM categories WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    emit categoriesChanged();
    return true;
}

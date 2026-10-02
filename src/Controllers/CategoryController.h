#pragma once

#include <QObject>
#include <QVector>

#include "Models/Category.h"

// Backed by Postgres (categories table). See ItemController for the query pattern.
class CategoryController : public QObject
{
    Q_OBJECT

public:
    explicit CategoryController(QObject *parent = nullptr);

    QVector<Category> allCategories() const;
    Category categoryById(int id) const;

    QString lastError() const { return m_lastError; }

public slots:
    bool addCategory(Category &category);
    bool updateCategory(const Category &category);
    bool removeCategory(int id);

signals:
    void categoriesChanged();

private:
    mutable QString m_lastError;
};

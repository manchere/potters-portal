#pragma once

#include <QObject>
#include <QVector>

#include "Models/Tag.h"

// Backed by Postgres (tags table). See ItemController for the query pattern.
class TagController : public QObject
{
    Q_OBJECT

public:
    explicit TagController(QObject *parent = nullptr);

    QVector<Tag> allTags() const;
    Tag tagById(int id) const;

    QString lastError() const { return m_lastError; }

public slots:
    bool addTag(Tag &tag);
    bool updateTag(const Tag &tag);
    bool removeTag(int id);

signals:
    void tagsChanged();

private:
    mutable QString m_lastError;
};

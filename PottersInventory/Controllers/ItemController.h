#pragma once

#include <QByteArray>
#include <QObject>
#include <QVector>

#include "Models/Item.h"

// Backed by Postgres (items + item_tags tables). Every method issues a
// synchronous QSqlQuery against QSqlDatabase::database() and reports
// failures through lastError(); callers should check the bool return value.
class ItemController : public QObject
{
    Q_OBJECT

public:
    explicit ItemController(QObject *parent = nullptr);

    QVector<Item> allItems() const;
    Item itemById(int id) const;
    // Returns an item with id() < 0 if no item has this barcode.
    Item itemByBarcode(const QString &barcode) const;
    QVector<int> tagIdsForItem(int itemId) const;
    // Loads the actual image bytes (not carried on Item — see Models/Item.h).
    // Returns false / leaves data empty if the item has no image.
    bool itemImage(int id, QByteArray &data, QString &mime) const;

    QString lastError() const { return m_lastError; }

public slots:
    bool addItem(Item &item);
    bool updateItem(const Item &item);
    bool removeItem(int id);
    bool setItemStatus(int id, ItemStatus status);
    bool setItemImage(int id, const QByteArray &data, const QString &mime);
    bool setItemTags(int itemId, const QVector<int> &tagIds);
    bool addTagToItem(int itemId, int tagId);
    bool removeTagFromItem(int itemId, int tagId);

signals:
    void itemsChanged();

private:
    mutable QString m_lastError;
};

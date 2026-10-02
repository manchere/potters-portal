#include "ItemController.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "Database/Database.h"

ItemController::ItemController(QObject *parent)
    : QObject(parent)
{
}

static Item itemFromQuery(const QSqlQuery &query)
{
    Item item;
    item.setId(query.value(QStringLiteral("id")).toInt());
    item.setName(query.value(QStringLiteral("name")).toString());
    item.setDescription(query.value(QStringLiteral("description")).toString());
    item.setQuantity(query.value(QStringLiteral("quantity")).toInt());
    item.setLocation(query.value(QStringLiteral("location")).toString());
    item.setBarcode(query.value(QStringLiteral("barcode")).toString());
    item.setStatus(itemStatusFromString(query.value(QStringLiteral("status")).toString()));
    const QVariant categoryId = query.value(QStringLiteral("category_id"));
    item.setCategoryId(categoryId.isNull() ? -1 : categoryId.toInt());
    item.setImageMime(query.value(QStringLiteral("image_mime")).toString());
    return item;
}

QVector<Item> ItemController::allItems() const
{
    QVector<Item> items;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT id, name, description, quantity, location, barcode, status, category_id, image_mime "
        "FROM items ORDER BY name"));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return items;
    }
    while (query.next()) {
        Item item = itemFromQuery(query);
        item.setTagIds(tagIdsForItem(item.id()));
        items.append(item);
    }
    return items;
}

Item ItemController::itemById(int id) const
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT id, name, description, quantity, location, barcode, status, category_id, image_mime "
        "FROM items WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return Item();
    }
    Item item = itemFromQuery(query);
    item.setTagIds(tagIdsForItem(item.id()));
    return item;
}

Item ItemController::itemByBarcode(const QString &barcode) const
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT id, name, description, quantity, location, barcode, status, category_id, image_mime "
        "FROM items WHERE barcode = :barcode"));
    query.bindValue(QStringLiteral(":barcode"), barcode);
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return Item();
    }
    Item item = itemFromQuery(query);
    item.setTagIds(tagIdsForItem(item.id()));
    return item;
}

QVector<int> ItemController::tagIdsForItem(int itemId) const
{
    QVector<int> tagIds;
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT tag_id FROM item_tags WHERE item_id = :itemId"));
    query.bindValue(QStringLiteral(":itemId"), itemId);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return tagIds;
    }
    while (query.next()) {
        tagIds.append(query.value(0).toInt());
    }
    return tagIds;
}

bool ItemController::addItem(Item &item)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT INTO items (name, description, quantity, location, barcode, status, category_id) "
        "VALUES (:name, :description, :quantity, :location, :barcode, :status, :categoryId) "
        "RETURNING id"));
    query.bindValue(QStringLiteral(":name"), item.name());
    query.bindValue(QStringLiteral(":description"), item.description());
    query.bindValue(QStringLiteral(":quantity"), item.quantity());
    query.bindValue(QStringLiteral(":location"), item.location());
    query.bindValue(QStringLiteral(":barcode"), item.barcode().isEmpty() ? QVariant() : QVariant(item.barcode()));
    query.bindValue(QStringLiteral(":status"), itemStatusToString(item.status()));
    query.bindValue(QStringLiteral(":categoryId"), item.categoryId() > 0 ? QVariant(item.categoryId()) : QVariant());
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return false;
    }
    const int newId = query.value(0).toInt();
    if (!setItemTags(newId, item.tagIds())) {
        return false;
    }
    item.setId(newId);
    emit itemsChanged();
    return true;
}

bool ItemController::updateItem(const Item &item)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "UPDATE items SET name = :name, description = :description, quantity = :quantity, "
        "location = :location, barcode = :barcode, status = :status, category_id = :categoryId, updated_at = now() "
        "WHERE id = :id"));
    query.bindValue(QStringLiteral(":name"), item.name());
    query.bindValue(QStringLiteral(":description"), item.description());
    query.bindValue(QStringLiteral(":quantity"), item.quantity());
    query.bindValue(QStringLiteral(":location"), item.location());
    query.bindValue(QStringLiteral(":barcode"), item.barcode().isEmpty() ? QVariant() : QVariant(item.barcode()));
    query.bindValue(QStringLiteral(":status"), itemStatusToString(item.status()));
    query.bindValue(QStringLiteral(":categoryId"), item.categoryId() > 0 ? QVariant(item.categoryId()) : QVariant());
    query.bindValue(QStringLiteral(":id"), item.id());
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    if (!setItemTags(item.id(), item.tagIds())) {
        return false;
    }
    emit itemsChanged();
    return true;
}

bool ItemController::removeItem(int id)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM items WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    emit itemsChanged();
    return true;
}

bool ItemController::setItemStatus(int id, ItemStatus status)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("UPDATE items SET status = :status, updated_at = now() WHERE id = :id"));
    query.bindValue(QStringLiteral(":status"), itemStatusToString(status));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    emit itemsChanged();
    return true;
}

bool ItemController::setItemImage(int id, const QByteArray &data, const QString &mime)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "UPDATE items SET image_data = :data, image_mime = :mime, updated_at = now() WHERE id = :id"));
    query.bindValue(QStringLiteral(":data"), data);
    query.bindValue(QStringLiteral(":mime"), mime);
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    emit itemsChanged();
    return true;
}

bool ItemController::itemImage(int id, QByteArray &data, QString &mime) const
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT image_data, image_mime FROM items WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return false;
    }
    data = query.value(0).toByteArray();
    mime = query.value(1).toString();
    return !data.isEmpty();
}

bool ItemController::setItemTags(int itemId, const QVector<int> &tagIds)
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.transaction()) {
        m_lastError = db.lastError().text();
        return false;
    }

    QSqlQuery clear(db);
    clear.prepare(QStringLiteral("DELETE FROM item_tags WHERE item_id = :itemId"));
    clear.bindValue(QStringLiteral(":itemId"), itemId);
    if (!clear.exec()) {
        m_lastError = clear.lastError().text();
        db.rollback();
        return false;
    }

    for (int tagId : tagIds) {
        QSqlQuery insert(db);
        insert.prepare(QStringLiteral("INSERT INTO item_tags (item_id, tag_id) VALUES (:itemId, :tagId)"));
        insert.bindValue(QStringLiteral(":itemId"), itemId);
        insert.bindValue(QStringLiteral(":tagId"), tagId);
        if (!insert.exec()) {
            m_lastError = insert.lastError().text();
            db.rollback();
            return false;
        }
    }

    if (!db.commit()) {
        m_lastError = db.lastError().text();
        return false;
    }
    return true;
}

bool ItemController::addTagToItem(int itemId, int tagId)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT INTO item_tags (item_id, tag_id) VALUES (:itemId, :tagId) "
        "ON CONFLICT DO NOTHING"));
    query.bindValue(QStringLiteral(":itemId"), itemId);
    query.bindValue(QStringLiteral(":tagId"), tagId);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    emit itemsChanged();
    return true;
}

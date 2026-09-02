#pragma once

#include <QString>
#include <QVector>

enum class ItemStatus
{
    Available,
    Missing,
    Broken,
    Lost
};

// Matches the CHECK constraint on items.status in database/migrations/0004_create_items.sql.
QString itemStatusToString(ItemStatus status);
ItemStatus itemStatusFromString(const QString &status);
QVector<ItemStatus> allItemStatuses();

class Item
{
public:
    Item() = default;
    Item(int id, QString name, QString description, int quantity, int categoryId);

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }

    QString description() const { return m_description; }
    void setDescription(const QString &description) { m_description = description; }

    int quantity() const { return m_quantity; }
    void setQuantity(int quantity) { m_quantity = quantity; }

    QString location() const { return m_location; }
    void setLocation(const QString &location) { m_location = location; }

    // Scanned barcode/QR value, empty when the item has none. Used by the
    // mobile app to look an item up by scan instead of by name.
    QString barcode() const { return m_barcode; }
    void setBarcode(const QString &barcode) { m_barcode = barcode; }

    ItemStatus status() const { return m_status; }
    void setStatus(ItemStatus status) { m_status = status; }

    int categoryId() const { return m_categoryId; }
    void setCategoryId(int categoryId) { m_categoryId = categoryId; }

    QVector<int> tagIds() const { return m_tagIds; }
    void setTagIds(const QVector<int> &tagIds) { m_tagIds = tagIds; }

    // MIME type of the stored image (e.g. "image/jpeg"), empty when the item
    // has no image. The image bytes themselves live in Postgres
    // (items.image_data) but are deliberately not loaded onto Item — list
    // queries would otherwise pull every item's full image into memory.
    // Fetch/set the bytes via ItemController::itemImage()/setItemImage().
    QString imageMime() const { return m_imageMime; }
    void setImageMime(const QString &imageMime) { m_imageMime = imageMime; }

private:
    int m_id = -1;
    QString m_name;
    QString m_description;
    int m_quantity = 0;
    QString m_location;
    QString m_barcode;
    ItemStatus m_status = ItemStatus::Available;
    int m_categoryId = -1;
    QVector<int> m_tagIds;
    QString m_imageMime;
};

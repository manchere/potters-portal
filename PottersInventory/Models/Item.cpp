#include "Item.h"

QString itemStatusToString(ItemStatus status)
{
    switch (status) {
    case ItemStatus::Available: return QStringLiteral("available");
    case ItemStatus::Missing: return QStringLiteral("missing");
    case ItemStatus::Broken: return QStringLiteral("broken");
    case ItemStatus::Lost: return QStringLiteral("lost");
    }
    return QStringLiteral("available");
}

ItemStatus itemStatusFromString(const QString &status)
{
    if (status == QStringLiteral("missing")) return ItemStatus::Missing;
    if (status == QStringLiteral("broken")) return ItemStatus::Broken;
    if (status == QStringLiteral("lost")) return ItemStatus::Lost;
    return ItemStatus::Available;
}

QVector<ItemStatus> allItemStatuses()
{
    return {ItemStatus::Available, ItemStatus::Missing, ItemStatus::Broken, ItemStatus::Lost};
}

Item::Item(int id, QString name, QString description, int quantity, int categoryId)
    : m_id(id)
    , m_name(std::move(name))
    , m_description(std::move(description))
    , m_quantity(quantity)
    , m_categoryId(categoryId)
{
}

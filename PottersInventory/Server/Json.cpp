#include "Json.h"

namespace Json
{
    QJsonObject itemToJson(const Item &item)
    {
        QJsonArray tagIds;
        for (int id : item.tagIds()) {
            tagIds.append(id);
        }

        QJsonObject json;
        json[QStringLiteral("id")] = item.id();
        json[QStringLiteral("name")] = item.name();
        json[QStringLiteral("description")] = item.description();
        json[QStringLiteral("quantity")] = item.quantity();
        json[QStringLiteral("location")] = item.location();
        json[QStringLiteral("barcode")] = item.barcode().isEmpty() ? QJsonValue() : QJsonValue(item.barcode());
        json[QStringLiteral("status")] = itemStatusToString(item.status());
        json[QStringLiteral("category_id")] = item.categoryId() > 0 ? QJsonValue(item.categoryId()) : QJsonValue();
        json[QStringLiteral("tag_ids")] = tagIds;
        json[QStringLiteral("image_url")] = item.imageMime().isEmpty()
            ? QJsonValue()
            : QJsonValue(QStringLiteral("/api/items/%1/image").arg(item.id()));
        return json;
    }

    Item itemFromJson(const QJsonObject &json)
    {
        Item item;
        item.setId(json.value(QStringLiteral("id")).toInt(-1));
        item.setName(json.value(QStringLiteral("name")).toString());
        item.setDescription(json.value(QStringLiteral("description")).toString());
        item.setQuantity(json.value(QStringLiteral("quantity")).toInt());
        item.setLocation(json.value(QStringLiteral("location")).toString());
        item.setBarcode(json.value(QStringLiteral("barcode")).toString());
        item.setStatus(itemStatusFromString(json.value(QStringLiteral("status")).toString(QStringLiteral("available"))));
        item.setCategoryId(json.value(QStringLiteral("category_id")).toInt(-1));

        QVector<int> tagIds;
        for (const QJsonValue &value : json.value(QStringLiteral("tag_ids")).toArray()) {
            tagIds.append(value.toInt());
        }
        item.setTagIds(tagIds);
        return item;
    }

    QJsonObject tagToJson(const Tag &tag)
    {
        QJsonObject json;
        json[QStringLiteral("id")] = tag.id();
        json[QStringLiteral("name")] = tag.name();
        json[QStringLiteral("color")] = tag.color();
        json[QStringLiteral("description")] = tag.description();
        return json;
    }

    QJsonObject categoryToJson(const Category &category)
    {
        QJsonObject json;
        json[QStringLiteral("id")] = category.id();
        json[QStringLiteral("name")] = category.name();
        return json;
    }

    QJsonObject userToJson(const User &user)
    {
        QJsonObject json;
        json[QStringLiteral("id")] = user.id();
        json[QStringLiteral("name")] = user.name();
        json[QStringLiteral("email")] = user.email();
        json[QStringLiteral("is_admin")] = user.isAdmin();
        json[QStringLiteral("avatar_seed")] = user.avatarSeed();
        return json;
    }

    QJsonObject assignmentToJson(const Assignment &assignment, const RoleType &role)
    {
        QJsonObject json;
        json[QStringLiteral("id")] = assignment.id();
        json[QStringLiteral("role_id")] = assignment.roleId();
        json[QStringLiteral("role_name")] = role.name();
        json[QStringLiteral("role_icon")] = role.icon();
        json[QStringLiteral("service_date")] = assignment.serviceDate().toString(Qt::ISODate);
        json[QStringLiteral("member_id")] = assignment.memberId() > 0 ? QJsonValue(assignment.memberId()) : QJsonValue();
        json[QStringLiteral("support_member_id")] = assignment.supportMemberId() > 0
            ? QJsonValue(assignment.supportMemberId())
            : QJsonValue();
        json[QStringLiteral("notes")] = assignment.notes();
        return json;
    }

    QJsonObject availabilityMarkToJson(const AvailabilityMark &mark)
    {
        QJsonObject json;
        json[QStringLiteral("id")] = mark.id();
        json[QStringLiteral("user_id")] = mark.userId();
        json[QStringLiteral("date")] = mark.date().toString(Qt::ISODate);
        return json;
    }

    QJsonObject nonAvailabilityRequestToJson(const NonAvailabilityRequest &request)
    {
        QJsonObject json;
        json[QStringLiteral("id")] = request.id();
        json[QStringLiteral("assignment_id")] = request.assignmentId();
        json[QStringLiteral("user_id")] = request.userId();
        json[QStringLiteral("message")] = request.message();
        json[QStringLiteral("status")] = requestStatusToString(request.status());
        json[QStringLiteral("decided_by")] = request.decidedBy() > 0 ? QJsonValue(request.decidedBy()) : QJsonValue();
        json[QStringLiteral("decided_at")] = request.decidedAt().isValid()
            ? QJsonValue(request.decidedAt().toString(Qt::ISODate))
            : QJsonValue();
        return json;
    }

}

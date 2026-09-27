#pragma once

#include <QJsonArray>
#include <QJsonObject>

#include "Models/Assignment.h"
#include "Models/AvailabilityMark.h"
#include "Models/Category.h"
#include "Models/Item.h"
#include "Models/NonAvailabilityRequest.h"
#include "Models/RoleType.h"
#include "Models/Tag.h"
#include "Models/User.h"

// Converts between the plain C++ models and their JSON wire format, shared
// by every REST endpoint in Server.cpp. Field names are the same snake_case
// used by the Postgres columns so a client can round-trip a fetched object
// straight back into a PUT/POST body.
namespace Json
{
    QJsonObject itemToJson(const Item &item);
    Item itemFromJson(const QJsonObject &json);

    QJsonObject tagToJson(const Tag &tag);
    Tag tagFromJson(const QJsonObject &json);

    QJsonObject categoryToJson(const Category &category);
    Category categoryFromJson(const QJsonObject &json);

    // Never includes password_hash/password_salt -- those never leave the
    // server (see UserController).
    QJsonObject userToJson(const User &user);

    // Embeds role_name/role_icon (resolved from the given RoleType)
    // alongside role_id, so mobile can render a role's icon/name without a
    // separate lookup endpoint.
    QJsonObject assignmentToJson(const Assignment &assignment, const RoleType &role);

    QJsonObject availabilityMarkToJson(const AvailabilityMark &mark);

    QJsonObject nonAvailabilityRequestToJson(const NonAvailabilityRequest &request);
}

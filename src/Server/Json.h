#pragma once

#include <QJsonArray>
#include <QJsonObject>

#include "Models/AccessRights.h"
#include "Models/Duty.h"
#include "Models/AvailabilityMark.h"
#include "Models/Category.h"
#include "Models/Item.h"
#include "Models/NonAvailabilityRequest.h"
#include "Models/DutyType.h"
#include "Models/Feedback.h"
#include "Models/ScheduleReportRow.h"
#include "Models/Song.h"
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

    QJsonObject categoryToJson(const Category &category);

    // Never includes password_hash/password_salt -- those never leave the
    // server (see UserController).
    QJsonObject userToJson(const User &user);

    // Embeds duty_type_name/duty_type_icon (resolved from the given DutyType)
    // alongside duty_type_id, so mobile can render a duty type's icon/name without a
    // separate lookup endpoint.
    QJsonObject dutyToJson(const Duty &duty, const DutyType &dutyType);

    QJsonObject availabilityMarkToJson(const AvailabilityMark &mark);

    QJsonObject nonAvailabilityRequestToJson(const NonAvailabilityRequest &request);

    QJsonObject dutyTypeToJson(const DutyType &dutyType);

    QJsonObject songToJson(const Song &song);
    Song songFromJson(const QJsonObject &json);

    QJsonObject feedbackToJson(const Feedback &feedback);

    QJsonObject reportRowToJson(const ScheduleReportRow &row);

    // {"view": bool, "create": bool, "update": bool, "delete": bool}
    QJsonObject sectionAccessToJson(const SectionAccess &access);
}

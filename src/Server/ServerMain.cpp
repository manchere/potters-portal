// REST API for the mobile app, reusing the same Models/Controllers/Database
// code as the desktop app. Every route below is a thin wrapper around an
// existing Controller method, so desktop and mobile go through identical
// business logic and validation.
#include <QCoreApplication>
#include <QDate>
#include <QHttpServer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QTcpServer>
#include <QUrlQuery>

#include <optional>

#include "Auth/PasswordAuth.h"
#include "Controllers/AccessController.h"
#include "Controllers/DutyController.h"
#include "Controllers/DutyTypeController.h"
#include "Controllers/AvailabilityController.h"
#include "Controllers/CategoryController.h"
#include "Controllers/FeedbackController.h"
#include "Controllers/ItemController.h"
#include "Controllers/NonAvailabilityRequestController.h"
#include "Controllers/SessionController.h"
#include "Controllers/SongController.h"
#include "Controllers/TagController.h"
#include "Controllers/UserController.h"
#include "Database/Database.h"
#include "Models/MemberColors.h"
#include "AuthMiddleware.h"
#include "Json.h"
#include "Vision/GroqVisionClient.h"

using StatusCode = QHttpServerResponder::StatusCode;

static QHttpServerResponse errorResponse(const QString &message, StatusCode status)
{
    return QHttpServerResponse(QJsonObject{{QStringLiteral("error"), message}}, status);
}

// Accepts a data URL ("data:image/jpeg;base64,...") or bare base64 and
// decodes it. mime is set from the data URL header, defaulting to
// image/jpeg if there isn't one. Returns an empty byte array on failure.
static QByteArray decodeImageBase64(const QString &imageBase64, QString &mime)
{
    mime = QStringLiteral("image/jpeg");
    QString payload = imageBase64;
    const int commaIndex = imageBase64.indexOf(QLatin1Char(','));
    if (imageBase64.startsWith(QLatin1String("data:")) && commaIndex >= 0) {
        const QString header = imageBase64.left(commaIndex);
        const int colonIndex = header.indexOf(QLatin1Char(':'));
        const int semicolonIndex = header.indexOf(QLatin1Char(';'));
        if (colonIndex >= 0 && semicolonIndex > colonIndex) {
            mime = header.mid(colonIndex + 1, semicolonIndex - colonIndex - 1);
        }
        payload = imageBase64.mid(commaIndex + 1);
    }
    return QByteArray::fromBase64(payload.toLatin1());
}

// The Sunday whose duties count towards someone's access rights -- today if
// it's Sunday, else the next one (same as the desktop's MainWindow).
static QDate upcomingSunday()
{
    const QDate today = QDate::currentDate();
    return today.addDays(7 - today.dayOfWeek());
}

// A duty with who serves and who backs up (FR-7.4) and their color badges,
// so the app can show a line-up without looking members up itself.
static QJsonObject dutyWithPeople(const Duty &duty, const DutyType &dutyType, const UserController &users)
{
    QJsonObject json = Json::dutyToJson(duty, dutyType);
    const User member = duty.memberId() > 0 ? users.userById(duty.memberId()) : User();
    const User support = duty.supportMemberId() > 0 ? users.userById(duty.supportMemberId()) : User();
    json[QStringLiteral("member_name")] = member.id() >= 0 ? QJsonValue(member.name()) : QJsonValue();
    json[QStringLiteral("member_color")] = member.id() >= 0 ? QJsonValue(member.color()) : QJsonValue();
    json[QStringLiteral("support_member_name")] = support.id() >= 0 ? QJsonValue(support.name()) : QJsonValue();
    json[QStringLiteral("support_member_color")] = support.id() >= 0 ? QJsonValue(support.color()) : QJsonValue();
    return json;
}

// A duty from a POST/PUT body. Ids that are missing or null come out as -1
// ("nobody"), as the controllers expect.
static Duty dutyFromJson(const QJsonObject &json)
{
    Duty duty;
    duty.setDutyTypeId(json.value(QStringLiteral("duty_type_id")).toInt(-1));
    duty.setServiceDate(QDate::fromString(json.value(QStringLiteral("service_date")).toString(), Qt::ISODate));
    duty.setMemberId(json.value(QStringLiteral("member_id")).toInt(-1));
    duty.setSupportMemberId(json.value(QStringLiteral("support_member_id")).toInt(-1));
    duty.setNotes(json.value(QStringLiteral("notes")).toString().trimmed());
    return duty;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    const QString databaseUrl = qEnvironmentVariable("DATABASE_URL");
    if (databaseUrl.isEmpty()) {
        qCritical("DATABASE_URL is not set. Set it to a Postgres connection string and restart.");
        return 1;
    }
    QString connectError;
    if (!Database::connect(databaseUrl, &connectError)) {
        qCritical("Failed to connect to the database: %s", qPrintable(connectError));
        return 1;
    }

    ItemController itemController;
    TagController tagController;
    CategoryController categoryController;
    UserController userController;
    SessionController sessionController;
    DutyController dutyController;
    DutyTypeController dutyTypeController;
    AvailabilityController availabilityController;
    NonAvailabilityRequestController requestController;
    AccessController accessController;
    SongController songController;
    FeedbackController feedbackController;
    QNetworkAccessManager networkManager;

    QHttpServer server;

    // Every route that follows Settings > Access Rights starts with this:
    // signs the caller in from their token and checks they may do `action`
    // in `section`. Returns the error to send back, or nothing if allowed
    // (outUser is then the signed-in member).
    const auto denied = [&sessionController, &userController, &accessController](
                            const QHttpServerRequest &request, Section section, AccessAction action,
                            User &outUser) -> std::optional<QHttpServerResponse> {
        if (!requireAuth(request, sessionController, userController, outUser)) {
            return errorResponse(QStringLiteral("authentication required"), StatusCode::Unauthorized);
        }
        if (!accessController.rightsFor(outUser, upcomingSunday()).section(section).allows(action)) {
            return errorResponse(QStringLiteral("you don't have access to do this"), StatusCode::Forbidden);
        }
        return std::nullopt;
    };
    // Schedule changes have no access rule: Admins only, as on the desktop.
    const auto deniedUnlessAdmin = [&sessionController, &userController](
                                       const QHttpServerRequest &request) -> std::optional<QHttpServerResponse> {
        User user;
        if (!requireAuth(request, sessionController, userController, user)) {
            return errorResponse(QStringLiteral("authentication required"), StatusCode::Unauthorized);
        }
        if (!user.isAdmin()) {
            return errorResponse(QStringLiteral("only an Admin can change the schedule"), StatusCode::Forbidden);
        }
        return std::nullopt;
    };

    // --- CORS ---------------------------------------------------------------
    // Lets the web version of the mobile app (served from another address)
    // call this API from a browser. Requests are authorised by the bearer
    // token, not cookies, so allowing any origin exposes nothing extra.
    const auto addCorsHeaders = [](QHttpHeaders &headers) {
        headers.replaceOrAppend(QByteArrayLiteral("Access-Control-Allow-Origin"), QByteArrayLiteral("*"));
        headers.replaceOrAppend(QByteArrayLiteral("Access-Control-Allow-Methods"),
                                QByteArrayLiteral("GET, POST, PUT, PATCH, DELETE, OPTIONS"));
        headers.replaceOrAppend(QByteArrayLiteral("Access-Control-Allow-Headers"),
                                QByteArrayLiteral("Authorization, Content-Type"));
        headers.replaceOrAppend(QByteArrayLiteral("Access-Control-Max-Age"), QByteArrayLiteral("86400"));
    };
    server.addAfterRequestHandler(&server, [addCorsHeaders](const QHttpServerRequest &, QHttpServerResponse &response) {
        QHttpHeaders headers = response.headers();
        addCorsHeaders(headers);
        response.setHeaders(std::move(headers));
    });
    // A browser asks first (OPTIONS) before e.g. a POST with a token; no
    // route handles OPTIONS, so answer it here. Anything else is a 404.
    server.setMissingHandler(&server, [addCorsHeaders](const QHttpServerRequest &request, QHttpServerResponder &responder) {
        QHttpServerResponse response = request.method() == QHttpServerRequest::Method::Options
            ? QHttpServerResponse(StatusCode::NoContent)
            : errorResponse(QStringLiteral("not found"), StatusCode::NotFound);
        QHttpHeaders headers = response.headers();
        addCorsHeaders(headers);
        response.setHeaders(std::move(headers));
        responder.sendResponse(std::move(response));
    });

    // --- Health ------------------------------------------------------------
    // For the host's health check (and waking a sleeping free instance);
    // doesn't touch the database.
    server.route("/api/health", QHttpServerRequest::Method::Get, [] {
        return QHttpServerResponse(QJsonObject{{QStringLiteral("status"), QStringLiteral("ok")}});
    });

    // --- Items -------------------------------------------------------------
    server.route("/api/items", QHttpServerRequest::Method::Get, [&itemController] {
        QJsonArray items;
        for (const Item &item : itemController.allItems()) {
            items.append(Json::itemToJson(item));
        }
        return QHttpServerResponse(items);
    });

    server.route("/api/items/<arg>", QHttpServerRequest::Method::Get, [&itemController](int id) {
        const Item item = itemController.itemById(id);
        if (item.id() < 0) {
            return errorResponse(QStringLiteral("item not found"), StatusCode::NotFound);
        }
        return QHttpServerResponse(Json::itemToJson(item));
    });

    // Barcode/QR lookup for the mobile app's scan-to-identify flow. Must be
    // registered before "/api/items/<arg>" would otherwise be ambiguous;
    // QHttpServer resolves this fine since that route's <arg> is typed int
    // and only matches numeric ids.
    server.route("/api/items/barcode/<arg>", QHttpServerRequest::Method::Get, [&itemController](const QString &barcode) {
        const Item item = itemController.itemByBarcode(barcode);
        if (item.id() < 0) {
            return errorResponse(QStringLiteral("no item with this barcode"), StatusCode::NotFound);
        }
        return QHttpServerResponse(Json::itemToJson(item));
    });

    server.route("/api/items", QHttpServerRequest::Method::Post, [&itemController, &denied](const QHttpServerRequest &request) {
        User currentUser;
        if (auto error = denied(request, Section::Inventory, AccessAction::Create, currentUser)) {
            return std::move(*error);
        }
        const QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if (!doc.isObject()) {
            return errorResponse(QStringLiteral("expected a JSON object"), StatusCode::BadRequest);
        }
        Item item = Json::itemFromJson(doc.object());
        if (item.name().trimmed().isEmpty()) {
            return errorResponse(QStringLiteral("name is required"), StatusCode::BadRequest);
        }
        if (!itemController.addItem(item)) {
            return errorResponse(itemController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(Json::itemToJson(item), StatusCode::Created);
    });

    server.route("/api/items/<arg>", QHttpServerRequest::Method::Put, [&itemController, &denied](int id, const QHttpServerRequest &request) {
        User currentUser;
        if (auto error = denied(request, Section::Inventory, AccessAction::Update, currentUser)) {
            return std::move(*error);
        }
        const QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if (!doc.isObject()) {
            return errorResponse(QStringLiteral("expected a JSON object"), StatusCode::BadRequest);
        }
        Item item = Json::itemFromJson(doc.object());
        item.setId(id);
        if (item.name().trimmed().isEmpty()) {
            return errorResponse(QStringLiteral("name is required"), StatusCode::BadRequest);
        }
        if (!itemController.updateItem(item)) {
            return errorResponse(itemController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(Json::itemToJson(item));
    });

    server.route("/api/items/<arg>", QHttpServerRequest::Method::Delete, [&itemController, &denied](int id, const QHttpServerRequest &request) {
        User currentUser;
        if (auto error = denied(request, Section::Inventory, AccessAction::Delete, currentUser)) {
            return std::move(*error);
        }
        if (!itemController.removeItem(id)) {
            return errorResponse(itemController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(QJsonObject{{QStringLiteral("ok"), true}});
    });

    // A photo comes with a new item or replaces one, so either right will do.
    server.route("/api/items/<arg>/image", QHttpServerRequest::Method::Post, [&itemController, &denied](int id, const QHttpServerRequest &request) {
        User currentUser;
        if (auto error = denied(request, Section::Inventory, AccessAction::Create, currentUser);
            error && denied(request, Section::Inventory, AccessAction::Update, currentUser)) {
            return std::move(*error);
        }
        if (itemController.itemById(id).id() < 0) {
            return errorResponse(QStringLiteral("item not found"), StatusCode::NotFound);
        }
        const QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if (!doc.isObject() || !doc.object().contains(QStringLiteral("image_base64"))) {
            return errorResponse(QStringLiteral("expected {\"image_base64\": \"...\"}"), StatusCode::BadRequest);
        }
        QString mime;
        const QByteArray bytes = decodeImageBase64(doc.object().value(QStringLiteral("image_base64")).toString(), mime);
        if (bytes.isEmpty()) {
            return errorResponse(QStringLiteral("could not decode the image"), StatusCode::BadRequest);
        }
        if (!itemController.setItemImage(id, bytes, mime)) {
            return errorResponse(itemController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(Json::itemToJson(itemController.itemById(id)));
    });

    server.route("/api/items/<arg>/image", QHttpServerRequest::Method::Get, [&itemController](int id) {
        QByteArray data;
        QString mime;
        if (!itemController.itemImage(id, data, mime)) {
            return QHttpServerResponse(StatusCode::NotFound);
        }
        return QHttpServerResponse(mime.toUtf8(), data);
    });

    server.route("/api/items/<arg>/status", QHttpServerRequest::Method::Patch, [&itemController, &denied](int id, const QHttpServerRequest &request) {
        User currentUser;
        if (auto error = denied(request, Section::Inventory, AccessAction::Update, currentUser)) {
            return std::move(*error);
        }
        const QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if (!doc.isObject() || !doc.object().contains(QStringLiteral("status"))) {
            return errorResponse(QStringLiteral("expected {\"status\": \"...\"}"), StatusCode::BadRequest);
        }
        const ItemStatus status = itemStatusFromString(doc.object().value(QStringLiteral("status")).toString());
        if (!itemController.setItemStatus(id, status)) {
            return errorResponse(itemController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(Json::itemToJson(itemController.itemById(id)));
    });

    // --- Vision (Groq) -------------------------------------------------------
    // Suggests a name/description from a photo; the client fills the Add Item
    // form with the result for the user to review/edit, it never saves
    // anything on its own.
    server.route("/api/vision/describe-item", QHttpServerRequest::Method::Post,
                 [&networkManager](const QHttpServerRequest &request) {
        const QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if (!doc.isObject() || !doc.object().contains(QStringLiteral("image_base64"))) {
            return errorResponse(QStringLiteral("expected {\"image_base64\": \"...\"}"), StatusCode::BadRequest);
        }
        QString errorMessage;
        const QJsonObject suggestion = GroqVision::describeItem(
            networkManager, doc.object().value(QStringLiteral("image_base64")).toString(), &errorMessage);
        if (suggestion.isEmpty()) {
            return errorResponse(errorMessage, StatusCode::BadGateway);
        }
        return QHttpServerResponse(suggestion);
    });

    // --- Tags and categories (read-only; managed from the desktop app) ----
    server.route("/api/tags", QHttpServerRequest::Method::Get, [&tagController] {
        QJsonArray tags;
        for (const Tag &tag : tagController.allTags()) {
            tags.append(Json::tagToJson(tag));
        }
        return QHttpServerResponse(tags);
    });

    server.route("/api/categories", QHttpServerRequest::Method::Get, [&categoryController] {
        QJsonArray categories;
        for (const Category &category : categoryController.allCategories()) {
            categories.append(Json::categoryToJson(category));
        }
        return QHttpServerResponse(categories);
    });

    // --- Auth ----------------------------------------------------------------
    // Mobile-only: the desktop app authenticates in-process via
    // UserController::verifyPassword and never hits these routes.
    server.route("/api/auth/register", QHttpServerRequest::Method::Post,
                 [&userController, &sessionController](const QHttpServerRequest &request) {
        const QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if (!doc.isObject()) {
            return errorResponse(QStringLiteral("expected a JSON object"), StatusCode::BadRequest);
        }
        const QJsonObject body = doc.object();
        const QString name = body.value(QStringLiteral("name")).toString().trimmed();
        const QString email = body.value(QStringLiteral("email")).toString().trimmed();
        const QString password = body.value(QStringLiteral("password")).toString();
        // Optional; profiles created without one get the default color.
        const QString color = body.value(QStringLiteral("color")).toString(MemberColors::defaultColor());
        if (!MemberColors::isValid(color)) {
            return errorResponse(QStringLiteral("color must be one of the profile colors"), StatusCode::BadRequest);
        }
        if (name.isEmpty() || email.isEmpty() || password.length() < 8) {
            return errorResponse(
                QStringLiteral("name, email, and a password of at least 8 characters are required"),
                StatusCode::BadRequest);
        }
        if (userController.userByEmail(email).id() >= 0) {
            return errorResponse(QStringLiteral("an account with this email already exists"), StatusCode::Conflict);
        }
        User user;
        user.setName(name);
        user.setEmail(email);
        user.setIsAdmin(false);
        user.setColor(color.toLower());
        const QString salt = PasswordAuth::generateSalt();
        user.setPasswordSalt(salt);
        user.setPasswordHash(PasswordAuth::hashPassword(password, salt));
        if (!userController.addUser(user)) {
            return errorResponse(userController.lastError(), StatusCode::InternalServerError);
        }
        const QString token = sessionController.createSession(user.id());
        if (token.isEmpty()) {
            return errorResponse(sessionController.lastError(), StatusCode::InternalServerError);
        }
        QJsonObject responseBody = Json::userToJson(user);
        responseBody[QStringLiteral("token")] = token;
        return QHttpServerResponse(responseBody, StatusCode::Created);
    });

    server.route("/api/auth/login", QHttpServerRequest::Method::Post,
                 [&userController, &sessionController](const QHttpServerRequest &request) {
        const QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if (!doc.isObject()) {
            return errorResponse(QStringLiteral("expected a JSON object"), StatusCode::BadRequest);
        }
        const QString email = doc.object().value(QStringLiteral("email")).toString();
        const QString password = doc.object().value(QStringLiteral("password")).toString();
        User user;
        if (!userController.verifyPassword(email, password, user)) {
            return errorResponse(QStringLiteral("invalid email or password"), StatusCode::Unauthorized);
        }
        const QString token = sessionController.createSession(user.id());
        if (token.isEmpty()) {
            return errorResponse(sessionController.lastError(), StatusCode::InternalServerError);
        }
        QJsonObject responseBody = Json::userToJson(user);
        responseBody[QStringLiteral("token")] = token;
        return QHttpServerResponse(responseBody);
    });

    // Who the stored token belongs to: lets the app confirm a saved sign-in
    // at launch (and refresh the profile) without asking for the password.
    server.route("/api/auth/me", QHttpServerRequest::Method::Get,
                 [&sessionController, &userController](const QHttpServerRequest &request) {
        User currentUser;
        if (!requireAuth(request, sessionController, userController, currentUser)) {
            return errorResponse(QStringLiteral("authentication required"), StatusCode::Unauthorized);
        }
        return QHttpServerResponse(Json::userToJson(currentUser));
    });

    server.route("/api/auth/logout", QHttpServerRequest::Method::Post,
                 [&sessionController](const QHttpServerRequest &request) {
        const QString token = bearerToken(request);
        if (!token.isEmpty()) {
            sessionController.revoke(token);
        }
        return QHttpServerResponse(QJsonObject{{QStringLiteral("ok"), true}});
    });

    // --- Duties (mobile: "my duties" only; Admin CRUD/scheduling
    // stays in-process on the desktop app via DutyController directly,
    // same as Items/Tags/Categories) -----------------------------------------
    server.route("/api/duties/me", QHttpServerRequest::Method::Get,
                 [&sessionController, &userController, &dutyController, &dutyTypeController,
                  &availabilityController, &requestController](const QHttpServerRequest &request) {
        User currentUser;
        if (!requireAuth(request, sessionController, userController, currentUser)) {
            return errorResponse(QStringLiteral("authentication required"), StatusCode::Unauthorized);
        }
        QJsonArray duties;
        for (const Duty &duty : dutyController.dutiesForMember(currentUser.id())) {
            QJsonObject json = dutyWithPeople(duty, dutyTypeController.dutyTypeById(duty.dutyTypeId()), userController);
            // FR-4.2: flag when this duty's date collides with a mark
            // the member already made on their general calendar, so the
            // mobile client can prompt them to file a formal request.
            json[QStringLiteral("conflicts_with_calendar")] =
                availabilityController.isMarked(currentUser.id(), duty.serviceDate());
            // Which of serving / backing up this member is.
            json[QStringLiteral("role")] = duty.memberId() == currentUser.id() ? QStringLiteral("serving")
                                                                               : QStringLiteral("backup");
            const NonAvailabilityRequest existing =
                requestController.requestForDutyAndUser(duty.id(), currentUser.id());
            json[QStringLiteral("non_availability_request")] = existing.id() >= 0
                ? QJsonValue(Json::nonAvailabilityRequestToJson(existing))
                : QJsonValue();
            duties.append(json);
        }
        return QHttpServerResponse(duties);
    });

    // --- Availability (general calendar, FR-3) --------------------------------
    server.route("/api/availability", QHttpServerRequest::Method::Get,
                 [&sessionController, &userController, &availabilityController](const QHttpServerRequest &request) {
        User currentUser;
        if (!requireAuth(request, sessionController, userController, currentUser)) {
            return errorResponse(QStringLiteral("authentication required"), StatusCode::Unauthorized);
        }
        QJsonArray marks;
        for (const AvailabilityMark &mark : availabilityController.listForUser(currentUser.id())) {
            marks.append(Json::availabilityMarkToJson(mark));
        }
        return QHttpServerResponse(marks);
    });

    server.route("/api/availability", QHttpServerRequest::Method::Post,
                 [&sessionController, &userController, &availabilityController](const QHttpServerRequest &request) {
        User currentUser;
        if (!requireAuth(request, sessionController, userController, currentUser)) {
            return errorResponse(QStringLiteral("authentication required"), StatusCode::Unauthorized);
        }
        const QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if (!doc.isObject() || !doc.object().contains(QStringLiteral("date"))) {
            return errorResponse(QStringLiteral("expected {\"date\": \"YYYY-MM-DD\"}"), StatusCode::BadRequest);
        }
        const QDate date = QDate::fromString(doc.object().value(QStringLiteral("date")).toString(), Qt::ISODate);
        if (!date.isValid()) {
            return errorResponse(QStringLiteral("invalid date"), StatusCode::BadRequest);
        }
        if (!availabilityController.markUnavailable(currentUser.id(), date)) {
            return errorResponse(availabilityController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(QJsonObject{{QStringLiteral("ok"), true}}, StatusCode::Created);
    });

    server.route("/api/availability/<arg>", QHttpServerRequest::Method::Delete,
                 [&sessionController, &userController, &availabilityController]
                 (const QString &dateStr, const QHttpServerRequest &request) {
        User currentUser;
        if (!requireAuth(request, sessionController, userController, currentUser)) {
            return errorResponse(QStringLiteral("authentication required"), StatusCode::Unauthorized);
        }
        const QDate date = QDate::fromString(dateStr, Qt::ISODate);
        if (!date.isValid()) {
            return errorResponse(QStringLiteral("invalid date"), StatusCode::BadRequest);
        }
        if (!availabilityController.unmark(currentUser.id(), date)) {
            return errorResponse(availabilityController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(QJsonObject{{QStringLiteral("ok"), true}});
    });

    // --- Non-availability requests (FR-4, mobile-submitted; Admin
    // approve/deny stays in-process on the desktop app) -----------------------
    server.route("/api/duties/<arg>/non-availability-requests", QHttpServerRequest::Method::Post,
                 [&sessionController, &userController, &dutyController, &requestController]
                 (int dutyId, const QHttpServerRequest &request) {
        User currentUser;
        if (!requireAuth(request, sessionController, userController, currentUser)) {
            return errorResponse(QStringLiteral("authentication required"), StatusCode::Unauthorized);
        }
        const Duty duty = dutyController.dutyById(dutyId);
        if (duty.id() < 0) {
            return errorResponse(QStringLiteral("duty not found"), StatusCode::NotFound);
        }
        // FR-4.1: only valid once the Admin has actually assigned this user
        // (as primary or support) to this duty.
        if (duty.memberId() != currentUser.id() && duty.supportMemberId() != currentUser.id()) {
            return errorResponse(QStringLiteral("you are not assigned to this duty"), StatusCode::Forbidden);
        }
        const QJsonDocument doc = QJsonDocument::fromJson(request.body());
        const QString message = doc.isObject()
            ? doc.object().value(QStringLiteral("message")).toString().trimmed()
            : QString();
        if (message.isEmpty()) {
            return errorResponse(QStringLiteral("a message explaining the reason is required"), StatusCode::BadRequest);
        }
        NonAvailabilityRequest newRequest;
        newRequest.setDutyId(dutyId);
        newRequest.setUserId(currentUser.id());
        newRequest.setMessage(message);
        if (!requestController.create(newRequest)) {
            return errorResponse(requestController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(Json::nonAvailabilityRequestToJson(newRequest), StatusCode::Created);
    });

    server.route("/api/non-availability-requests/me", QHttpServerRequest::Method::Get,
                 [&sessionController, &userController, &requestController, &dutyController,
                  &dutyTypeController](const QHttpServerRequest &request) {
        User currentUser;
        if (!requireAuth(request, sessionController, userController, currentUser)) {
            return errorResponse(QStringLiteral("authentication required"), StatusCode::Unauthorized);
        }
        QJsonArray requests;
        for (const NonAvailabilityRequest &req : requestController.listForUser(currentUser.id())) {
            QJsonObject json = Json::nonAvailabilityRequestToJson(req);
            // Which duty it's about, so the list can say "Singing, Sun 4 Oct".
            const Duty duty = dutyController.dutyById(req.dutyId());
            if (duty.id() >= 0) {
                const DutyType dutyType = dutyTypeController.dutyTypeById(duty.dutyTypeId());
                json[QStringLiteral("duty_type_name")] = dutyType.name();
                json[QStringLiteral("duty_type_icon")] = dutyType.icon();
                json[QStringLiteral("service_date")] = duty.serviceDate().toString(Qt::ISODate);
            }
            requests.append(json);
        }
        return QHttpServerResponse(requests);
    });

    // --- Access rights ---------------------------------------------------------
    // What the signed-in member may do, per section, so the app shows only
    // the screens and buttons they can use. The routes check again anyway.
    server.route("/api/access/me", QHttpServerRequest::Method::Get,
                 [&sessionController, &userController, &accessController](const QHttpServerRequest &request) {
        User currentUser;
        if (!requireAuth(request, sessionController, userController, currentUser)) {
            return errorResponse(QStringLiteral("authentication required"), StatusCode::Unauthorized);
        }
        const AccessRights rights = accessController.rightsFor(currentUser, upcomingSunday());
        QJsonObject sections;
        for (Section section : allSections()) {
            sections[sectionKey(section)] = Json::sectionAccessToJson(rights.section(section));
        }
        return QHttpServerResponse(QJsonObject{
            {QStringLiteral("is_admin"), currentUser.isAdmin()},
            {QStringLiteral("can_manage_schedule"), currentUser.isAdmin()},
            {QStringLiteral("sections"), sections},
        });
    });

    // --- Schedule (everyone can see it; only Admins change it) -----------------
    server.route("/api/schedule/<arg>", QHttpServerRequest::Method::Get,
                 [&sessionController, &userController, &dutyController, &dutyTypeController]
                 (const QString &dateStr, const QHttpServerRequest &request) {
        User currentUser;
        if (!requireAuth(request, sessionController, userController, currentUser)) {
            return errorResponse(QStringLiteral("authentication required"), StatusCode::Unauthorized);
        }
        const QDate date = QDate::fromString(dateStr, Qt::ISODate);
        if (!date.isValid()) {
            return errorResponse(QStringLiteral("invalid date"), StatusCode::BadRequest);
        }
        QJsonArray duties;
        for (const Duty &duty : dutyController.dutiesForDate(date)) {
            duties.append(dutyWithPeople(duty, dutyTypeController.dutyTypeById(duty.dutyTypeId()), userController));
        }
        return QHttpServerResponse(QJsonObject{
            {QStringLiteral("date"), date.toString(Qt::ISODate)},
            // Past Sundays are read-only for everyone (DutyController).
            {QStringLiteral("editable"), DutyController::isEditableDate(date)},
            {QStringLiteral("duties"), duties},
        });
    });

    // The pickers for adding a duty: duty types and members (names and
    // colors only -- no emails).
    server.route("/api/duty-types", QHttpServerRequest::Method::Get,
                 [&sessionController, &userController, &dutyTypeController](const QHttpServerRequest &request) {
        User currentUser;
        if (!requireAuth(request, sessionController, userController, currentUser)) {
            return errorResponse(QStringLiteral("authentication required"), StatusCode::Unauthorized);
        }
        QJsonArray dutyTypes;
        for (const DutyType &dutyType : dutyTypeController.allDutyTypes()) {
            dutyTypes.append(Json::dutyTypeToJson(dutyType));
        }
        return QHttpServerResponse(dutyTypes);
    });

    server.route("/api/members", QHttpServerRequest::Method::Get,
                 [&sessionController, &userController](const QHttpServerRequest &request) {
        User currentUser;
        if (!requireAuth(request, sessionController, userController, currentUser)) {
            return errorResponse(QStringLiteral("authentication required"), StatusCode::Unauthorized);
        }
        QJsonArray members;
        for (const User &user : userController.allUsers()) {
            members.append(QJsonObject{
                {QStringLiteral("id"), user.id()},
                {QStringLiteral("name"), user.name()},
                {QStringLiteral("color"), user.color()},
            });
        }
        return QHttpServerResponse(members);
    });

    server.route("/api/duties", QHttpServerRequest::Method::Post,
                 [&dutyController, &dutyTypeController, &userController, &deniedUnlessAdmin](const QHttpServerRequest &request) {
        if (auto error = deniedUnlessAdmin(request)) {
            return std::move(*error);
        }
        const QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if (!doc.isObject()) {
            return errorResponse(QStringLiteral("expected a JSON object"), StatusCode::BadRequest);
        }
        Duty duty = dutyFromJson(doc.object());
        if (!duty.serviceDate().isValid() || duty.serviceDate().dayOfWeek() != Qt::Sunday) {
            return errorResponse(QStringLiteral("service_date must be a Sunday"), StatusCode::BadRequest);
        }
        const DutyType dutyType = dutyTypeController.dutyTypeById(duty.dutyTypeId());
        if (dutyType.id() < 0) {
            return errorResponse(QStringLiteral("pick a duty"), StatusCode::BadRequest);
        }
        if (!dutyController.addDuty(duty)) {
            return errorResponse(dutyController.lastError(), StatusCode::BadRequest);
        }
        return QHttpServerResponse(dutyWithPeople(duty, dutyType, userController), StatusCode::Created);
    });

    // Changes who serves, the backup and the notes; the duty and its Sunday
    // stay as they are.
    server.route("/api/duties/<arg>", QHttpServerRequest::Method::Put,
                 [&dutyController, &dutyTypeController, &userController, &deniedUnlessAdmin]
                 (int id, const QHttpServerRequest &request) {
        if (auto error = deniedUnlessAdmin(request)) {
            return std::move(*error);
        }
        Duty duty = dutyController.dutyById(id);
        if (duty.id() < 0) {
            return errorResponse(QStringLiteral("duty not found"), StatusCode::NotFound);
        }
        const QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if (!doc.isObject()) {
            return errorResponse(QStringLiteral("expected a JSON object"), StatusCode::BadRequest);
        }
        const Duty changes = dutyFromJson(doc.object());
        duty.setMemberId(changes.memberId());
        duty.setSupportMemberId(changes.supportMemberId());
        duty.setNotes(changes.notes());
        if (!dutyController.updateDuty(duty)) {
            return errorResponse(dutyController.lastError(), StatusCode::BadRequest);
        }
        return QHttpServerResponse(dutyWithPeople(duty, dutyTypeController.dutyTypeById(duty.dutyTypeId()), userController));
    });

    server.route("/api/duties/<arg>", QHttpServerRequest::Method::Delete,
                 [&dutyController, &deniedUnlessAdmin](int id, const QHttpServerRequest &request) {
        if (auto error = deniedUnlessAdmin(request)) {
            return std::move(*error);
        }
        if (!dutyController.removeDuty(id)) {
            return errorResponse(dutyController.lastError(), StatusCode::BadRequest);
        }
        return QHttpServerResponse(QJsonObject{{QStringLiteral("ok"), true}});
    });

    // --- Reports ("who did what", like the desktop Reports tab) ---------------
    // ?from=YYYY-MM-DD&to=YYYY-MM-DD; newest Sunday first.
    server.route("/api/reports/schedule", QHttpServerRequest::Method::Get,
                 [&dutyController, &denied](const QHttpServerRequest &request) {
        User currentUser;
        if (auto error = denied(request, Section::Reports, AccessAction::View, currentUser)) {
            return std::move(*error);
        }
        const QUrlQuery query(request.url());
        QDate from = QDate::fromString(query.queryItemValue(QStringLiteral("from")), Qt::ISODate);
        QDate to = QDate::fromString(query.queryItemValue(QStringLiteral("to")), Qt::ISODate);
        if (!from.isValid() || !to.isValid()) {
            return errorResponse(QStringLiteral("expected ?from=YYYY-MM-DD&to=YYYY-MM-DD"), StatusCode::BadRequest);
        }
        if (from > to) {
            std::swap(from, to);
        }
        QJsonArray rows;
        for (const ScheduleReportRow &row : dutyController.scheduleReport(from, to)) {
            rows.append(Json::reportRowToJson(row));
        }
        return QHttpServerResponse(rows);
    });

    // --- Songs ---------------------------------------------------------------
    server.route("/api/songs", QHttpServerRequest::Method::Get,
                 [&songController, &denied](const QHttpServerRequest &request) {
        User currentUser;
        if (auto error = denied(request, Section::Songs, AccessAction::View, currentUser)) {
            return std::move(*error);
        }
        QJsonArray songs;
        for (const Song &song : songController.allSongs()) {
            songs.append(Json::songToJson(song));
        }
        return QHttpServerResponse(songs);
    });

    server.route("/api/songs", QHttpServerRequest::Method::Post,
                 [&songController, &denied](const QHttpServerRequest &request) {
        User currentUser;
        if (auto error = denied(request, Section::Songs, AccessAction::Create, currentUser)) {
            return std::move(*error);
        }
        const QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if (!doc.isObject()) {
            return errorResponse(QStringLiteral("expected a JSON object"), StatusCode::BadRequest);
        }
        Song song = Json::songFromJson(doc.object());
        if (song.title().isEmpty()) {
            return errorResponse(QStringLiteral("title is required"), StatusCode::BadRequest);
        }
        if (!songController.addSong(song)) {
            return errorResponse(songController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(Json::songToJson(song), StatusCode::Created);
    });

    server.route("/api/songs/<arg>", QHttpServerRequest::Method::Put,
                 [&songController, &denied](int id, const QHttpServerRequest &request) {
        User currentUser;
        if (auto error = denied(request, Section::Songs, AccessAction::Update, currentUser)) {
            return std::move(*error);
        }
        const QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if (!doc.isObject()) {
            return errorResponse(QStringLiteral("expected a JSON object"), StatusCode::BadRequest);
        }
        Song song = Json::songFromJson(doc.object());
        song.setId(id);
        if (song.title().isEmpty()) {
            return errorResponse(QStringLiteral("title is required"), StatusCode::BadRequest);
        }
        if (!songController.updateSong(song)) {
            return errorResponse(songController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(Json::songToJson(song));
    });

    server.route("/api/songs/<arg>", QHttpServerRequest::Method::Delete,
                 [&songController, &denied](int id, const QHttpServerRequest &request) {
        User currentUser;
        if (auto error = denied(request, Section::Songs, AccessAction::Delete, currentUser)) {
            return std::move(*error);
        }
        if (!songController.removeSong(id)) {
            return errorResponse(songController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(QJsonObject{{QStringLiteral("ok"), true}});
    });

    // --- Feedback --------------------------------------------------------------
    // Sending needs Feedback "create"; reading everyone's requests needs
    // "update" or "delete", as on the desktop Feedback tab.
    server.route("/api/feedback", QHttpServerRequest::Method::Post,
                 [&feedbackController, &denied](const QHttpServerRequest &request) {
        User currentUser;
        if (auto error = denied(request, Section::Feedback, AccessAction::Create, currentUser)) {
            return std::move(*error);
        }
        const QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if (!doc.isObject()) {
            return errorResponse(QStringLiteral("expected a JSON object"), StatusCode::BadRequest);
        }
        const QJsonObject body = doc.object();
        Feedback feedback;
        feedback.setKind(Feedback::kindFromKey(body.value(QStringLiteral("kind")).toString()));
        feedback.setMemberId(currentUser.id());
        feedback.setSubject(body.value(QStringLiteral("subject")).toString().trimmed());
        feedback.setDetails(body.value(QStringLiteral("details")).toString().trimmed());
        if (feedback.subject().isEmpty()) {
            return errorResponse(QStringLiteral("a subject is required"), StatusCode::BadRequest);
        }
        if (!feedbackController.addFeedback(feedback)) {
            return errorResponse(feedbackController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(Json::feedbackToJson(feedback), StatusCode::Created);
    });

    server.route("/api/feedback", QHttpServerRequest::Method::Get,
                 [&feedbackController, &userController, &denied](const QHttpServerRequest &request) {
        User currentUser;
        if (auto error = denied(request, Section::Feedback, AccessAction::Update, currentUser);
            error && denied(request, Section::Feedback, AccessAction::Delete, currentUser)) {
            return std::move(*error);
        }
        QJsonArray list;
        for (const Feedback &feedback : feedbackController.allFeedback()) {
            QJsonObject json = Json::feedbackToJson(feedback);
            const User member = feedback.memberId() > 0 ? userController.userById(feedback.memberId()) : User();
            json[QStringLiteral("member_name")] = member.id() >= 0 ? QJsonValue(member.name()) : QJsonValue();
            list.append(json);
        }
        return QHttpServerResponse(list);
    });

    server.route("/api/feedback/<arg>", QHttpServerRequest::Method::Patch,
                 [&feedbackController, &denied](int id, const QHttpServerRequest &request) {
        User currentUser;
        if (auto error = denied(request, Section::Feedback, AccessAction::Update, currentUser)) {
            return std::move(*error);
        }
        const QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if (!doc.isObject() || !doc.object().contains(QStringLiteral("done"))) {
            return errorResponse(QStringLiteral("expected {\"done\": true|false}"), StatusCode::BadRequest);
        }
        if (!feedbackController.setDone(id, doc.object().value(QStringLiteral("done")).toBool())) {
            return errorResponse(feedbackController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(QJsonObject{{QStringLiteral("ok"), true}});
    });

    server.route("/api/feedback/<arg>", QHttpServerRequest::Method::Delete,
                 [&feedbackController, &denied](int id, const QHttpServerRequest &request) {
        User currentUser;
        if (auto error = denied(request, Section::Feedback, AccessAction::Delete, currentUser)) {
            return std::move(*error);
        }
        if (!feedbackController.removeFeedback(id)) {
            return errorResponse(feedbackController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(QJsonObject{{QStringLiteral("ok"), true}});
    });

    auto *tcpServer = new QTcpServer(&app);
    const quint16 port = static_cast<quint16>(qEnvironmentVariableIntValue("PORT") > 0 ? qEnvironmentVariableIntValue("PORT") : 8080);
    if (!tcpServer->listen(QHostAddress::Any, port) || !server.bind(tcpServer)) {
        qCritical("Failed to listen on port %d", port);
        return 1;
    }
    qInfo("Potters Portal server listening on http://localhost:%d", port);

    return app.exec();
}

// REST API + static web frontend host, reusing the same Models/Controllers/
// Database code as the desktop app. Every route below is a thin wrapper
// around an existing Controller method, so the two frontends (desktop
// widgets, browser) and any future native mobile client all go through
// identical business logic and validation.
#include <QCoreApplication>
#include <QDate>
#include <QFile>
#include <QHttpServer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QTcpServer>

#include "Auth/PasswordAuth.h"
#include "Controllers/AssignmentController.h"
#include "Controllers/AvailabilityController.h"
#include "Controllers/CategoryController.h"
#include "Controllers/ItemController.h"
#include "Controllers/NonAvailabilityRequestController.h"
#include "Controllers/SessionController.h"
#include "Controllers/TagController.h"
#include "Controllers/UserController.h"
#include "Database/Database.h"
#include "AuthMiddleware.h"
#include "Json.h"
#include "Vision/GroqVisionClient.h"

using StatusCode = QHttpServerResponder::StatusCode;

static QHttpServerResponse errorResponse(const QString &message, StatusCode status)
{
    return QHttpServerResponse(QJsonObject{{QStringLiteral("error"), message}}, status);
}

static QString webRootDir()
{
    const QString override = qEnvironmentVariable("WEB_ROOT");
    if (!override.isEmpty()) {
        return override;
    }
    return QStringLiteral(POTTERS_WEB_ROOT);
}

static QHttpServerResponse serveStaticFile(const QString &relativePath, const QByteArray &contentType)
{
    QFile file(webRootDir() + QLatin1Char('/') + relativePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QHttpServerResponse(StatusCode::NotFound);
    }
    return QHttpServerResponse(contentType, file.readAll());
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
    AssignmentController assignmentController;
    AvailabilityController availabilityController;
    NonAvailabilityRequestController requestController;
    QNetworkAccessManager networkManager;

    QHttpServer server;

    // --- Static web frontend --------------------------------------------
    server.route("/", QHttpServerRequest::Method::Get, [] {
        return serveStaticFile(QStringLiteral("index.html"), "text/html; charset=utf-8");
    });
    server.route("/style.css", QHttpServerRequest::Method::Get, [] {
        return serveStaticFile(QStringLiteral("style.css"), "text/css; charset=utf-8");
    });
    server.route("/app.js", QHttpServerRequest::Method::Get, [] {
        return serveStaticFile(QStringLiteral("app.js"), "application/javascript; charset=utf-8");
    });
    server.route("/items.html", QHttpServerRequest::Method::Get, [] {
        return serveStaticFile(QStringLiteral("items.html"), "text/html; charset=utf-8");
    });
    server.route("/items.js", QHttpServerRequest::Method::Get, [] {
        return serveStaticFile(QStringLiteral("items.js"), "application/javascript; charset=utf-8");
    });
    server.route("/tags.html", QHttpServerRequest::Method::Get, [] {
        return serveStaticFile(QStringLiteral("tags.html"), "text/html; charset=utf-8");
    });
    server.route("/tags.js", QHttpServerRequest::Method::Get, [] {
        return serveStaticFile(QStringLiteral("tags.js"), "application/javascript; charset=utf-8");
    });
    server.route("/pwa.js", QHttpServerRequest::Method::Get, [] {
        return serveStaticFile(QStringLiteral("pwa.js"), "application/javascript; charset=utf-8");
    });
    server.route("/manifest.json", QHttpServerRequest::Method::Get, [] {
        return serveStaticFile(QStringLiteral("manifest.json"), "application/manifest+json; charset=utf-8");
    });
    server.route("/service-worker.js", QHttpServerRequest::Method::Get, [] {
        return serveStaticFile(QStringLiteral("service-worker.js"), "application/javascript; charset=utf-8");
    });
    server.route("/icons/icon-192.png", QHttpServerRequest::Method::Get, [] {
        return serveStaticFile(QStringLiteral("icons/icon-192.png"), "image/png");
    });
    server.route("/icons/icon-512.png", QHttpServerRequest::Method::Get, [] {
        return serveStaticFile(QStringLiteral("icons/icon-512.png"), "image/png");
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

    server.route("/api/items", QHttpServerRequest::Method::Post, [&itemController](const QHttpServerRequest &request) {
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

    server.route("/api/items/<arg>", QHttpServerRequest::Method::Put, [&itemController](int id, const QHttpServerRequest &request) {
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

    server.route("/api/items/<arg>", QHttpServerRequest::Method::Delete, [&itemController](int id) {
        if (!itemController.removeItem(id)) {
            return errorResponse(itemController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(QJsonObject{{QStringLiteral("ok"), true}});
    });

    server.route("/api/items/<arg>/image", QHttpServerRequest::Method::Post, [&itemController](int id, const QHttpServerRequest &request) {
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

    server.route("/api/items/<arg>/status", QHttpServerRequest::Method::Patch, [&itemController](int id, const QHttpServerRequest &request) {
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

    server.route("/api/items/<arg>/tags/<arg>", QHttpServerRequest::Method::Post, [&itemController](int itemId, int tagId) {
        if (!itemController.addTagToItem(itemId, tagId)) {
            return errorResponse(itemController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(Json::itemToJson(itemController.itemById(itemId)));
    });

    server.route("/api/items/<arg>/tags/<arg>", QHttpServerRequest::Method::Delete, [&itemController](int itemId, int tagId) {
        if (!itemController.removeTagFromItem(itemId, tagId)) {
            return errorResponse(itemController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(Json::itemToJson(itemController.itemById(itemId)));
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

    // --- Tags ----------------------------------------------------------------
    server.route("/api/tags", QHttpServerRequest::Method::Get, [&tagController] {
        QJsonArray tags;
        for (const Tag &tag : tagController.allTags()) {
            tags.append(Json::tagToJson(tag));
        }
        return QHttpServerResponse(tags);
    });

    server.route("/api/tags", QHttpServerRequest::Method::Post, [&tagController](const QHttpServerRequest &request) {
        const QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if (!doc.isObject()) {
            return errorResponse(QStringLiteral("expected a JSON object"), StatusCode::BadRequest);
        }
        Tag tag = Json::tagFromJson(doc.object());
        if (tag.name().trimmed().isEmpty()) {
            return errorResponse(QStringLiteral("name is required"), StatusCode::BadRequest);
        }
        if (!tagController.addTag(tag)) {
            return errorResponse(tagController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(Json::tagToJson(tag), StatusCode::Created);
    });

    server.route("/api/tags/<arg>", QHttpServerRequest::Method::Put, [&tagController](int id, const QHttpServerRequest &request) {
        const QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if (!doc.isObject()) {
            return errorResponse(QStringLiteral("expected a JSON object"), StatusCode::BadRequest);
        }
        Tag tag = Json::tagFromJson(doc.object());
        tag.setId(id);
        if (!tagController.updateTag(tag)) {
            return errorResponse(tagController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(Json::tagToJson(tag));
    });

    server.route("/api/tags/<arg>", QHttpServerRequest::Method::Delete, [&tagController](int id) {
        if (!tagController.removeTag(id)) {
            return errorResponse(tagController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(QJsonObject{{QStringLiteral("ok"), true}});
    });

    // --- Categories ------------------------------------------------------------
    server.route("/api/categories", QHttpServerRequest::Method::Get, [&categoryController] {
        QJsonArray categories;
        for (const Category &category : categoryController.allCategories()) {
            categories.append(Json::categoryToJson(category));
        }
        return QHttpServerResponse(categories);
    });

    server.route("/api/categories", QHttpServerRequest::Method::Post, [&categoryController](const QHttpServerRequest &request) {
        const QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if (!doc.isObject()) {
            return errorResponse(QStringLiteral("expected a JSON object"), StatusCode::BadRequest);
        }
        Category category = Json::categoryFromJson(doc.object());
        if (category.name().trimmed().isEmpty()) {
            return errorResponse(QStringLiteral("name is required"), StatusCode::BadRequest);
        }
        if (!categoryController.addCategory(category)) {
            return errorResponse(categoryController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(Json::categoryToJson(category), StatusCode::Created);
    });

    server.route("/api/categories/<arg>", QHttpServerRequest::Method::Put, [&categoryController](int id, const QHttpServerRequest &request) {
        const QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if (!doc.isObject()) {
            return errorResponse(QStringLiteral("expected a JSON object"), StatusCode::BadRequest);
        }
        Category category = Json::categoryFromJson(doc.object());
        category.setId(id);
        if (!categoryController.updateCategory(category)) {
            return errorResponse(categoryController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(Json::categoryToJson(category));
    });

    server.route("/api/categories/<arg>", QHttpServerRequest::Method::Delete, [&categoryController](int id) {
        if (!categoryController.removeCategory(id)) {
            return errorResponse(categoryController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(QJsonObject{{QStringLiteral("ok"), true}});
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
        user.setAvatarSeed(name);
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

    server.route("/api/auth/logout", QHttpServerRequest::Method::Post,
                 [&sessionController](const QHttpServerRequest &request) {
        const QString token = bearerToken(request);
        if (!token.isEmpty()) {
            sessionController.revoke(token);
        }
        return QHttpServerResponse(QJsonObject{{QStringLiteral("ok"), true}});
    });

    // --- Assignments (mobile: "my assignments" only; Admin CRUD/scheduling
    // stays in-process on the desktop app via AssignmentController directly,
    // same as Items/Tags/Categories) -----------------------------------------
    server.route("/api/assignments/me", QHttpServerRequest::Method::Get,
                 [&sessionController, &userController, &assignmentController, &availabilityController,
                  &requestController](const QHttpServerRequest &request) {
        User currentUser;
        if (!requireAuth(request, sessionController, userController, currentUser)) {
            return errorResponse(QStringLiteral("authentication required"), StatusCode::Unauthorized);
        }
        QJsonArray assignments;
        for (const Assignment &assignment : assignmentController.assignmentsForMember(currentUser.id())) {
            QJsonObject json = Json::assignmentToJson(assignment);
            // FR-4.2: flag when this assignment's date collides with a mark
            // the member already made on their general calendar, so the
            // mobile client can prompt them to file a formal request.
            json[QStringLiteral("conflicts_with_calendar")] =
                availabilityController.isMarked(currentUser.id(), assignment.serviceDate());
            const NonAvailabilityRequest existing =
                requestController.requestForAssignmentAndUser(assignment.id(), currentUser.id());
            json[QStringLiteral("non_availability_request")] = existing.id() >= 0
                ? QJsonValue(Json::nonAvailabilityRequestToJson(existing))
                : QJsonValue();
            assignments.append(json);
        }
        return QHttpServerResponse(assignments);
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
    server.route("/api/assignments/<arg>/non-availability-requests", QHttpServerRequest::Method::Post,
                 [&sessionController, &userController, &assignmentController, &requestController]
                 (int assignmentId, const QHttpServerRequest &request) {
        User currentUser;
        if (!requireAuth(request, sessionController, userController, currentUser)) {
            return errorResponse(QStringLiteral("authentication required"), StatusCode::Unauthorized);
        }
        const Assignment assignment = assignmentController.assignmentById(assignmentId);
        if (assignment.id() < 0) {
            return errorResponse(QStringLiteral("assignment not found"), StatusCode::NotFound);
        }
        // FR-4.1: only valid once the Admin has actually assigned this user
        // (as primary or support) to this assignment.
        if (assignment.memberId() != currentUser.id() && assignment.supportMemberId() != currentUser.id()) {
            return errorResponse(QStringLiteral("you are not assigned to this assignment"), StatusCode::Forbidden);
        }
        const QJsonDocument doc = QJsonDocument::fromJson(request.body());
        const QString message = doc.isObject()
            ? doc.object().value(QStringLiteral("message")).toString().trimmed()
            : QString();
        if (message.isEmpty()) {
            return errorResponse(QStringLiteral("a message explaining the reason is required"), StatusCode::BadRequest);
        }
        NonAvailabilityRequest newRequest;
        newRequest.setAssignmentId(assignmentId);
        newRequest.setUserId(currentUser.id());
        newRequest.setMessage(message);
        if (!requestController.create(newRequest)) {
            return errorResponse(requestController.lastError(), StatusCode::InternalServerError);
        }
        return QHttpServerResponse(Json::nonAvailabilityRequestToJson(newRequest), StatusCode::Created);
    });

    server.route("/api/non-availability-requests/me", QHttpServerRequest::Method::Get,
                 [&sessionController, &userController, &requestController](const QHttpServerRequest &request) {
        User currentUser;
        if (!requireAuth(request, sessionController, userController, currentUser)) {
            return errorResponse(QStringLiteral("authentication required"), StatusCode::Unauthorized);
        }
        QJsonArray requests;
        for (const NonAvailabilityRequest &req : requestController.listForUser(currentUser.id())) {
            requests.append(Json::nonAvailabilityRequestToJson(req));
        }
        return QHttpServerResponse(requests);
    });

    auto *tcpServer = new QTcpServer(&app);
    const quint16 port = static_cast<quint16>(qEnvironmentVariableIntValue("PORT") > 0 ? qEnvironmentVariableIntValue("PORT") : 8080);
    if (!tcpServer->listen(QHostAddress::Any, port) || !server.bind(tcpServer)) {
        qCritical("Failed to listen on port %d", port);
        return 1;
    }
    qInfo("Potter's Inventory server listening on http://localhost:%d", port);

    return app.exec();
}

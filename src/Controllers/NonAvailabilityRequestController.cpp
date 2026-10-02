#include "NonAvailabilityRequestController.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "Database/Database.h"

namespace
{
    const QString kSelectColumns = QStringLiteral(
        "id, duty_id, user_id, message, status, decided_by, decided_at");
}

NonAvailabilityRequestController::NonAvailabilityRequestController(QObject *parent)
    : QObject(parent)
{
}

static NonAvailabilityRequest requestFromQuery(const QSqlQuery &query)
{
    NonAvailabilityRequest request;
    request.setId(query.value(QStringLiteral("id")).toInt());
    request.setDutyId(query.value(QStringLiteral("duty_id")).toInt());
    request.setUserId(query.value(QStringLiteral("user_id")).toInt());
    request.setMessage(query.value(QStringLiteral("message")).toString());
    request.setStatus(requestStatusFromString(query.value(QStringLiteral("status")).toString()));
    const QVariant decidedBy = query.value(QStringLiteral("decided_by"));
    request.setDecidedBy(decidedBy.isNull() ? -1 : decidedBy.toInt());
    request.setDecidedAt(query.value(QStringLiteral("decided_at")).toDateTime());
    return request;
}

QVector<NonAvailabilityRequest> NonAvailabilityRequestController::listForUser(int userId) const
{
    QVector<NonAvailabilityRequest> requests;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT %1 FROM non_availability_requests WHERE user_id = :user_id ORDER BY created_at DESC").arg(kSelectColumns));
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return requests;
    }
    while (query.next()) {
        requests.append(requestFromQuery(query));
    }
    return requests;
}

NonAvailabilityRequest NonAvailabilityRequestController::requestForDutyAndUser(int dutyId, int userId) const
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT %1 FROM non_availability_requests WHERE duty_id = :duty_id AND user_id = :user_id")
        .arg(kSelectColumns));
    query.bindValue(QStringLiteral(":duty_id"), dutyId);
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return NonAvailabilityRequest();
    }
    return requestFromQuery(query);
}

bool NonAvailabilityRequestController::create(NonAvailabilityRequest &request)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT INTO non_availability_requests (duty_id, user_id, message) "
        "VALUES (:duty_id, :user_id, :message) RETURNING id"));
    query.bindValue(QStringLiteral(":duty_id"), request.dutyId());
    query.bindValue(QStringLiteral(":user_id"), request.userId());
    query.bindValue(QStringLiteral(":message"), request.message());
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return false;
    }
    request.setId(query.value(0).toInt());
    request.setStatus(RequestStatus::Pending);
    emit requestsChanged();
    return true;
}

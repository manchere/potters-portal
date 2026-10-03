#include "NonAvailabilityRequestController.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "Database/Database.h"

namespace
{
    // Every query reads the table as "n", so the same columns work when
    // it's joined to duties.
    const QString kSelectColumns = QStringLiteral(
        "n.id, n.duty_id, n.user_id, n.message, n.status, n.decided_by, n.decided_at, n.created_at");
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
    request.setCreatedAt(query.value(QStringLiteral("created_at")).toDateTime());
    return request;
}

QVector<NonAvailabilityRequest> NonAvailabilityRequestController::listForUser(int userId) const
{
    QVector<NonAvailabilityRequest> requests;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT %1 FROM non_availability_requests n WHERE n.user_id = :user_id ORDER BY n.created_at DESC").arg(kSelectColumns));
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
        "SELECT %1 FROM non_availability_requests n WHERE n.duty_id = :duty_id AND n.user_id = :user_id")
        .arg(kSelectColumns));
    query.bindValue(QStringLiteral(":duty_id"), dutyId);
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return NonAvailabilityRequest();
    }
    return requestFromQuery(query);
}

QVector<NonAvailabilityRequest> NonAvailabilityRequestController::listPending() const
{
    QVector<NonAvailabilityRequest> requests;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT %1 FROM non_availability_requests n JOIN duties d ON d.id = n.duty_id "
        "WHERE n.status = 'pending' AND d.service_date >= CURRENT_DATE "
        "ORDER BY d.service_date, n.created_at").arg(kSelectColumns));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return requests;
    }
    while (query.next()) {
        requests.append(requestFromQuery(query));
    }
    return requests;
}

QVector<NonAvailabilityRequest> NonAvailabilityRequestController::listForDate(const QDate &date) const
{
    QVector<NonAvailabilityRequest> requests;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT %1 FROM non_availability_requests n JOIN duties d ON d.id = n.duty_id "
        "WHERE d.service_date = :date ORDER BY n.created_at").arg(kSelectColumns));
    query.bindValue(QStringLiteral(":date"), date);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return requests;
    }
    while (query.next()) {
        requests.append(requestFromQuery(query));
    }
    return requests;
}

bool NonAvailabilityRequestController::approve(int requestId, int adminId)
{
    return decide(requestId, RequestStatus::Approved, adminId);
}

bool NonAvailabilityRequestController::deny(int requestId, int adminId)
{
    return decide(requestId, RequestStatus::Denied, adminId);
}

bool NonAvailabilityRequestController::decide(int requestId, RequestStatus status, int adminId)
{
    Database::ensureConnected();
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.transaction()) {
        m_lastError = db.lastError().text();
        return false;
    }
    auto fail = [this, &db](const QString &error) {
        m_lastError = error;
        db.rollback();
        return false;
    };

    // Locks the request and its duty, so a second Admin deciding at the
    // same time (or an edit to the duty) waits for this one.
    QSqlQuery lookup;
    lookup.prepare(QStringLiteral(
        "SELECT n.status, n.user_id, d.id AS duty_id, d.service_date, d.member_id, d.support_member_id "
        "FROM non_availability_requests n JOIN duties d ON d.id = n.duty_id "
        "WHERE n.id = :id FOR UPDATE"));
    lookup.bindValue(QStringLiteral(":id"), requestId);
    if (!lookup.exec()) {
        return fail(lookup.lastError().text());
    }
    if (!lookup.next()) {
        return fail(tr("This request no longer exists. Its duty may have been deleted."));
    }
    if (requestStatusFromString(lookup.value(QStringLiteral("status")).toString()) != RequestStatus::Pending) {
        return fail(tr("This request has already been decided."));
    }
    if (lookup.value(QStringLiteral("service_date")).toDate() < QDate::currentDate()) {
        return fail(tr("This Sunday has passed, so its schedule can't be changed."));
    }

    if (status == RequestStatus::Approved) {
        const int userId = lookup.value(QStringLiteral("user_id")).toInt();
        const QVariant memberId = lookup.value(QStringLiteral("member_id"));
        const QVariant supportId = lookup.value(QStringLiteral("support_member_id"));
        QString sql;
        if (!memberId.isNull() && memberId.toInt() == userId) {
            // The backup serves instead; with no backup the duty is left
            // unfilled for the Admin to reassign.
            sql = QStringLiteral(
                "UPDATE duties SET member_id = support_member_id, support_member_id = NULL, updated_at = now() "
                "WHERE id = :duty_id");
        } else if (!supportId.isNull() && supportId.toInt() == userId) {
            sql = QStringLiteral(
                "UPDATE duties SET support_member_id = NULL, updated_at = now() WHERE id = :duty_id");
        }
        // Otherwise they were already taken off the duty: nothing to change.
        if (!sql.isEmpty()) {
            QSqlQuery update;
            update.prepare(sql);
            update.bindValue(QStringLiteral(":duty_id"), lookup.value(QStringLiteral("duty_id")));
            if (!update.exec()) {
                return fail(update.lastError().text());
            }
        }
    }

    QSqlQuery record;
    record.prepare(QStringLiteral(
        "UPDATE non_availability_requests SET status = :status, decided_by = :decided_by, "
        "decided_at = now(), updated_at = now() WHERE id = :id"));
    record.bindValue(QStringLiteral(":status"), requestStatusToString(status));
    record.bindValue(QStringLiteral(":decided_by"), adminId > 0 ? QVariant(adminId) : QVariant());
    record.bindValue(QStringLiteral(":id"), requestId);
    if (!record.exec()) {
        return fail(record.lastError().text());
    }

    if (!db.commit()) {
        return fail(db.lastError().text());
    }
    emit requestsChanged();
    return true;
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

#pragma once

#include <QDate>
#include <QObject>
#include <QVector>

#include "Models/NonAvailabilityRequest.h"

// Backed by Postgres (non_availability_requests table). See
// ItemController for the query pattern.
class NonAvailabilityRequestController : public QObject
{
    Q_OBJECT

public:
    explicit NonAvailabilityRequestController(QObject *parent = nullptr);

    QVector<NonAvailabilityRequest> listForUser(int userId) const;

    // Used to avoid a Member submitting a second request against the same
    // duty; returns an invalid (id < 0) request if none exists.
    NonAvailabilityRequest requestForDutyAndUser(int dutyId, int userId) const;

    // Pending requests against duties from today on, soonest Sunday first
    // (then oldest request first) -- the desktop Schedule tab's Admin
    // queue (FR-5.1). Ones for a Sunday that has passed are left out: that
    // schedule can no longer change.
    QVector<NonAvailabilityRequest> listPending() const;

    // Every request (any status) filed against a duty on date, oldest
    // first -- so the Schedule tab can say who asked to be absent.
    QVector<NonAvailabilityRequest> listForDate(const QDate &date) const;

    QString lastError() const { return m_lastError; }

public slots:
    // request.dutyId()/userId()/message() must be set by the caller;
    // status is always inserted as Pending regardless of what's set on
    // request (FR-4.4 -- a request always starts pending).
    bool create(NonAvailabilityRequest &request);

    // FR-5.2. Approving takes the requester off the duty in the same
    // transaction: if they were serving, their backup (if any) serves
    // instead and the backup slot empties; if they were the backup, the
    // backup slot empties. Both refuse a request that's no longer pending
    // or whose Sunday has passed. adminId is recorded as decided_by.
    bool approve(int requestId, int adminId);
    bool deny(int requestId, int adminId);

signals:
    void requestsChanged();

private:
    bool decide(int requestId, RequestStatus status, int adminId);

    mutable QString m_lastError;
};

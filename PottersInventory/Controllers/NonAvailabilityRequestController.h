#pragma once

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
    // assignment; returns an invalid (id < 0) request if none exists.
    NonAvailabilityRequest requestForAssignmentAndUser(int assignmentId, int userId) const;

    QString lastError() const { return m_lastError; }

public slots:
    // request.assignmentId()/userId()/message() must be set by the caller;
    // status is always inserted as Pending regardless of what's set on
    // request (FR-4.4 -- a request always starts pending).
    bool create(NonAvailabilityRequest &request);

signals:
    void requestsChanged();

private:
    mutable QString m_lastError;
};

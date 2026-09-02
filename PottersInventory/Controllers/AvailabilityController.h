#pragma once

#include <QDate>
#include <QObject>
#include <QVector>

#include "Models/AvailabilityMark.h"

// Backed by Postgres (availability_marks table). Member-only general
// calendar (FR-3.1/3.2) -- used by the mobile client via REST, not by the
// Admin desktop app.
class AvailabilityController : public QObject
{
    Q_OBJECT

public:
    explicit AvailabilityController(QObject *parent = nullptr);

    QVector<AvailabilityMark> listForUser(int userId) const;

    // Used to compute the FR-4.2 auto-flag: true if userId has marked date
    // unavailable on their general calendar.
    bool isMarked(int userId, const QDate &date) const;

    QString lastError() const { return m_lastError; }

public slots:
    bool markUnavailable(int userId, const QDate &date);
    bool unmark(int userId, const QDate &date);

private:
    mutable QString m_lastError;
};

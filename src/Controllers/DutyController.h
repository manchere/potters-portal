#pragma once

#include <QDate>
#include <QObject>
#include <QStringList>
#include <QVector>

#include "Models/Duty.h"
#include "Models/ScheduleReportRow.h"

// Backed by Postgres (duties table). See ItemController for the
// query pattern.
class DutyController : public QObject
{
    Q_OBJECT

public:
    explicit DutyController(QObject *parent = nullptr);

    QVector<Duty> allDuties() const;
    Duty dutyById(int id) const;

    // Duties where userId is either the primary or support member,
    // from today onward, soonest first (FR-2.1's "upcoming Sundays").
    QVector<Duty> dutiesForMember(int userId) const;

    // Every duty (past and upcoming) where userId is either the
    // primary or support member, most recent first -- used by the desktop
    // Member stats modal, unlike dutiesForMember() which is
    // upcoming-only for the mobile "my duties" API.
    QVector<Duty> allDutiesForMember(int userId) const;

    // Every duty on a specific date, for the desktop Schedule tab
    // (FR-8.1/8.2).
    QVector<Duty> dutiesForDate(const QDate &date) const;

    // Names of Members assigned on date (as primary or support) who have
    // also marked that date unavailable on their general calendar (FR-3.1)
    // -- surfaced after pasting a schedule so the Admin can swap them out.
    QStringList membersMarkedUnavailable(const QDate &date) const;

    // Every duty from fromDate to toDate inclusive, newest Sunday
    // first (then by duty type name), with duty type/member names and request /
    // availability flags resolved -- for the desktop Reports tab. Filters
    // (each empty means no filter):
    //  - memberIds / teamIds: duties where one of those members, or anyone
    //    on one of those teams, is primary or support (the two lists add up
    //    to one set of people);
    //  - dutyTypeIds: only those duty types.
    QVector<ScheduleReportRow> scheduleReport(const QDate &fromDate, const QDate &toDate,
                                              const QVector<int> &memberIds = {},
                                              const QVector<int> &dutyTypeIds = {},
                                              const QVector<int> &teamIds = {}) const;

    QString lastError() const { return m_lastError; }

    // A schedule can only be changed until its Sunday has passed (today
    // still counts). add/update/remove/copySchedule refuse anything that
    // would change a past schedule, whoever calls them.
    static bool isEditableDate(const QDate &date);

public slots:
    bool addDuty(Duty &duty);
    bool updateDuty(const Duty &duty);
    bool removeDuty(int id);

    // Copies every duty on fromDate (duty type, member, support member,
    // notes) onto toDate, in one transaction. With replaceExisting, toDate's
    // current duties are deleted first (along with any
    // non-availability requests filed against them, via ON DELETE
    // CASCADE); otherwise they're kept, and a source duty is skipped
    // when toDate already has the same duty type for the same member. copied
    // and skipped (either may be null) report what happened.
    bool copySchedule(const QDate &fromDate, const QDate &toDate, bool replaceExisting, int *copied, int *skipped);

signals:
    void dutiesChanged();

private:
    mutable QString m_lastError;
};

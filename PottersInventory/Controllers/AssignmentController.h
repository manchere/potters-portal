#pragma once

#include <QDate>
#include <QObject>
#include <QStringList>
#include <QVector>

#include "Models/Assignment.h"
#include "Models/ScheduleReportRow.h"

// Backed by Postgres (assignments table). See ItemController for the
// query pattern.
class AssignmentController : public QObject
{
    Q_OBJECT

public:
    explicit AssignmentController(QObject *parent = nullptr);

    QVector<Assignment> allAssignments() const;
    Assignment assignmentById(int id) const;

    // Assignments where userId is either the primary or support member,
    // from today onward, soonest first (FR-2.1's "upcoming Sundays").
    QVector<Assignment> assignmentsForMember(int userId) const;

    // Every assignment (past and upcoming) where userId is either the
    // primary or support member, most recent first -- used by the desktop
    // Member stats modal, unlike assignmentsForMember() which is
    // upcoming-only for the mobile "my assignments" API.
    QVector<Assignment> allAssignmentsForMember(int userId) const;

    // Every assignment on a specific date, for the desktop Date tab
    // (FR-8.1/8.2).
    QVector<Assignment> assignmentsForDate(const QDate &date) const;

    // Names of Members assigned on date (as primary or support) who have
    // also marked that date unavailable on their general calendar (FR-3.1)
    // -- surfaced after pasting a schedule so the Admin can swap them out.
    QStringList membersMarkedUnavailable(const QDate &date) const;

    // Every assignment from fromDate to toDate inclusive, newest Sunday
    // first (then by role name), with role/member names and request /
    // availability flags resolved -- for the desktop Reports tab. With
    // memberId > 0, only assignments where that member is primary or
    // support.
    QVector<ScheduleReportRow> scheduleReport(const QDate &fromDate, const QDate &toDate, int memberId = -1) const;

    QString lastError() const { return m_lastError; }

public slots:
    bool addAssignment(Assignment &assignment);
    bool updateAssignment(const Assignment &assignment);
    bool removeAssignment(int id);

    // Copies every assignment on fromDate (role, member, support member,
    // notes) onto toDate, in one transaction. With replaceExisting, toDate's
    // current assignments are deleted first (along with any
    // non-availability requests filed against them, via ON DELETE
    // CASCADE); otherwise they're kept, and a source assignment is skipped
    // when toDate already has the same role for the same member. copied
    // and skipped (either may be null) report what happened.
    bool copySchedule(const QDate &fromDate, const QDate &toDate, bool replaceExisting, int *copied, int *skipped);

signals:
    void assignmentsChanged();

private:
    mutable QString m_lastError;
};

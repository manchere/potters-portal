#pragma once

#include <QDate>
#include <QString>

// One assignment as it appears in a schedule report (desktop Reports tab):
// the Assignment plus the names and flags a report needs, resolved in a
// single query by AssignmentController::scheduleReport() instead of one
// lookup per row. Read-only -- nothing is ever saved from this.
struct ScheduleReportRow
{
    int assignmentId = -1;
    QDate serviceDate;
    QString roleName;
    QString roleIcon;
    // -1 / empty when the slot has no one assigned.
    int memberId = -1;
    QString memberName;
    int supportMemberId = -1;
    QString supportMemberName;
    QString notes;
    // Latest non-availability request the primary member filed against
    // this assignment: "pending", "approved", "denied", or empty for none.
    // Only the status -- the member's reason stays private.
    QString requestStatus;
    // The primary member marked this date unavailable on their calendar.
    bool memberMarkedUnavailable = false;
};

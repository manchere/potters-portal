#pragma once

#include <QDate>
#include <QString>

// One duty as it appears in a schedule report (desktop Reports tab):
// the Duty plus the names and flags a report needs, resolved in a
// single query by DutyController::scheduleReport() instead of one
// lookup per row. Read-only -- nothing is ever saved from this.
struct ScheduleReportRow
{
    int dutyId = -1;
    QDate serviceDate;
    QString dutyTypeName;
    QString dutyTypeIcon;
    // -1 / empty when the slot has no one assigned.
    int memberId = -1;
    QString memberName;
    int supportMemberId = -1;
    QString supportMemberName;
    QString notes;
    // Latest non-availability request the primary member filed against
    // this duty: "pending", "approved", "denied", or empty for none.
    // Only the status -- the member's reason stays private.
    QString requestStatus;
    // The primary member marked this date unavailable on their calendar.
    bool memberMarkedUnavailable = false;
};

#pragma once

#include <QDate>
#include <QString>
#include <QVector>

class QNetworkAccessManager;
class AvailabilityController;
class DutyController;
class DutyTypeController;
class UserController;

// "Fill with AI" on the Schedule tab (desktop and mobile): asks Groq who
// should serve on the gaps of upcoming Sundays, then checks every answer
// before the Admin sees it. Nothing is saved here -- the caller previews
// the proposals and saves the ones the Admin keeps (DutyController::addDuty
// on the desktop, POST /api/duties from the app).
//
// A gap is a chosen duty type with no duty yet on that Sunday; Sundays'
// existing duties are never changed. The model sees the last
// kHistoryMonths of duties (who usually does which duty, and when they
// last did), who's already on each Sunday, and who marked each Sunday
// unavailable (FR-3.1). Its answers are dropped when they name an unknown
// member or duty, someone marked away, someone already serving that
// Sunday, or a slot that isn't a gap; such slots come back as unfilled.
namespace AiSchedule
{
    struct Proposal
    {
        QDate date;
        int dutyTypeId = -1;
        int memberId = -1;
        int supportMemberId = -1; // -1: no backup
    };

    struct Slot
    {
        QDate date;
        int dutyTypeId = -1;
    };

    struct Result
    {
        QVector<Proposal> proposals; // sorted by date, then duty type name
        QVector<Slot> unfilled;      // gaps the model left empty or got wrong
        QString error;               // set (and both lists empty) on failure
    };

    // The next count Sundays that can still be edited, starting with the
    // upcoming one (today, if it's Sunday).
    QVector<QDate> upcomingSundays(int count);

    // dutyTypeIds empty means every duty type. instructions is optional
    // free text from the Admin ("Grace is leading worship all of November").
    Result propose(QNetworkAccessManager &manager,
                   const DutyController &duties,
                   const DutyTypeController &dutyTypes,
                   const UserController &users,
                   const AvailabilityController &availability,
                   const QVector<QDate> &sundays,
                   const QVector<int> &dutyTypeIds,
                   const QString &instructions);
}

#pragma once

#include "FramelessDialog.h"

#include <QVector>

#include "Models/Duty.h"
#include "Models/User.h"

class DutyTypeController;

// "Responsibilities" modal opened by double-clicking a member's profile
// row in ScheduleTab's results list -- a quick summary of
// everything they've ever been assigned (duty breakdown + full history),
// not just what's scheduled for the currently selected Sunday.
//
// Opened from the member's Sunday summary, it gets a "Back" button
// (showBack) that closes it with backRequested() true, so the caller can
// reopen that summary.
class MemberStatsDialog : public FramelessDialog
{
    Q_OBJECT

public:
    MemberStatsDialog(
        const User &user,
        const QVector<Duty> &duties,
        DutyTypeController *dutyTypeController,
        QWidget *parent = nullptr,
        bool showBack = false);

    bool backRequested() const { return m_backRequested; }

private:
    bool m_backRequested = false;
};

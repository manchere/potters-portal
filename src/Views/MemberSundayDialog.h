#pragma once

#include "FramelessDialog.h"

#include <QDate>
#include <QVector>

#include "Models/Duty.h"
#include "Models/User.h"

class DutyTypeController;
class UserController;

// Opened by double-clicking a member's row on the Schedule tab: who they
// are (badge, name, team, email) and their part in the selected Sunday --
// the duties they serve (each with its backup and notes), the duties they
// back someone else up on, and a warning if they marked themselves away.
//
// "All Duties..." asks the caller to open their full history
// (allDutiesRequested); "Edit on Schedule" appears only when the caller
// allows editing (an Admin on an upcoming Sunday) and closes the dialog
// with editRequested() true.
class MemberSundayDialog : public FramelessDialog
{
    Q_OBJECT

public:
    MemberSundayDialog(
        const User &member,
        const QString &teamName,
        const QDate &sunday,
        const QVector<Duty> &sundayDuties,
        bool markedAway,
        bool canEdit,
        UserController *userController,
        DutyTypeController *dutyTypeController,
        QWidget *parent = nullptr);

    bool editRequested() const { return m_editRequested; }
    bool allDutiesRequested() const { return m_allDutiesRequested; }

private:
    bool m_editRequested = false;
    bool m_allDutiesRequested = false;
};

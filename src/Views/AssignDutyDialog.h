#pragma once

#include "FramelessDialog.h"

#include <QDate>
#include <QVector>

#include "Models/Duty.h"
#include "Models/User.h"

class QComboBox;
class QPlainTextEdit;
class DutyTypeController;

// Create/edit a duty for a fixed Sunday -- the date already
// selected on the Schedule tab -- reached via the "Assign Duty" button there
// rather than a separate tab (SCHEDULING_FUNCTIONAL_REQUIREMENTS.md FR-6,
// FR-7). Mirrors CategoryEditDialog: pass an existing Duty to edit,
// or a default-constructed Duty() (id() < 0) for "new". The Duty
// dropdown is populated from DutyTypeController (admin-manageable duty
// types), not a fixed list.
class AssignDutyDialog : public FramelessDialog
{
    Q_OBJECT

public:
    AssignDutyDialog(
        const Duty &duty,
        const QDate &serviceDate,
        const QVector<User> &members,
        DutyTypeController *dutyTypeController,
        QWidget *parent = nullptr);

    Duty duty() const;

private:
    int m_id = -1;
    QDate m_serviceDate;
    QComboBox *m_dutyTypeCombo = nullptr;
    QComboBox *m_memberCombo = nullptr;
    QComboBox *m_supportMemberCombo = nullptr;
    QPlainTextEdit *m_notesEdit = nullptr;
};

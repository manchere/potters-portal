#pragma once

#include "FramelessDialog.h"

#include <QDate>
#include <QVector>

#include "Models/Duty.h"
#include "Models/User.h"

class QLabel;
class QPlainTextEdit;
class DutyTypeController;
class MemberPickerField;
class SuggestLineEdit;

// The Schedule tab's "+ Add Member": puts a Member on the open Sunday's
// schedule in one go -- pick their name, one or more duties, and an
// optional backup who covers if they're not around. Each duty becomes its
// own Duty row with the same Member, backup and notes. (Creating a new
// Member profile is done from the Taxonomy tab.)
//
// Given the Member's existing duties on that Sunday it edits them instead
// ("Edit Member on Schedule", opened by double-clicking them): the Member
// is fixed, and the duties, backup and notes start from what's there.
// duties() is then the wanted set; ScheduleTab works out what to add,
// change and remove.
class AddToScheduleDialog : public FramelessDialog
{
    Q_OBJECT

public:
    AddToScheduleDialog(
        const QDate &serviceDate,
        const QVector<User> &members,
        DutyTypeController *dutyTypeController,
        QWidget *parent = nullptr,
        const QVector<Duty> &existingDuties = {});

    // One Duty per chosen duty type (id() < 0), all with the chosen
    // Member, backup and notes.
    QVector<Duty> duties() const;

private slots:
    void saveClicked();

private:
    QDate m_serviceDate;
    SuggestLineEdit *m_memberEdit = nullptr;
    MemberPickerField *m_dutyPicker = nullptr;
    SuggestLineEdit *m_backupEdit = nullptr;
    QPlainTextEdit *m_notesEdit = nullptr;
    QLabel *m_errorLabel = nullptr;
};

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
class AddToScheduleDialog : public FramelessDialog
{
    Q_OBJECT

public:
    AddToScheduleDialog(
        const QDate &serviceDate,
        const QVector<User> &members,
        DutyTypeController *dutyTypeController,
        QWidget *parent = nullptr);

    // One Duty per chosen duty type (id() < 0, ready for addDuty()).
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

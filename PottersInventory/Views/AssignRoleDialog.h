#pragma once

#include "FramelessDialog.h"

#include <QDate>
#include <QVector>

#include "Models/Assignment.h"
#include "Models/User.h"

class QComboBox;
class QPlainTextEdit;

// Create/edit a role assignment for a fixed Sunday -- the date already
// selected on the Date tab -- reached via the "Assign Role" button there
// rather than a separate tab (SCHEDULING_FUNCTIONAL_REQUIREMENTS.md FR-6,
// FR-7). Mirrors CategoryEditDialog: pass an existing Assignment to edit,
// or a default-constructed Assignment() (id() < 0) for "new".
class AssignRoleDialog : public FramelessDialog
{
    Q_OBJECT

public:
    AssignRoleDialog(
        const Assignment &assignment,
        const QDate &serviceDate,
        const QVector<User> &members,
        QWidget *parent = nullptr);

    Assignment assignment() const;

private:
    int m_id = -1;
    QDate m_serviceDate;
    QComboBox *m_roleCombo = nullptr;
    QComboBox *m_memberCombo = nullptr;
    QComboBox *m_supportMemberCombo = nullptr;
    QPlainTextEdit *m_notesEdit = nullptr;
};

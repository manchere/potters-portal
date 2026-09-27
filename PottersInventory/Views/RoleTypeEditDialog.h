#pragma once

#include "FramelessDialog.h"

#include "Models/RoleType.h"

class QLabel;
class QLineEdit;

// Add/edit an assignment role *type* (name + a single emoji icon) --
// these are the duty types (Singing, Translating, ...) a Member can be
// scheduled against on the Date tab; this dialog only manages the type
// itself, never a scheduled assignment for a particular Member/date (that
// happens exclusively via AssignRoleDialog on the Date tab). Used by
// AdminOverviewView's Taxonomy tab -- pass an existing RoleType to edit,
// or a default-constructed RoleType() (id() < 0) for "new", mirroring
// TagEditDialog/CategoryEditDialog.
class RoleTypeEditDialog : public FramelessDialog
{
    Q_OBJECT

public:
    RoleTypeEditDialog(const RoleType &roleType, QWidget *parent = nullptr);

    RoleType roleType() const;

private slots:
    void saveClicked();

private:
    int m_id = -1;
    QLineEdit *m_nameEdit = nullptr;
    QLabel *m_nameError = nullptr;
    QLineEdit *m_iconEdit = nullptr;
};

#pragma once

#include "FramelessDialog.h"

#include "Models/DutyType.h"

class QLabel;
class QLineEdit;

// Add/edit a duty *type* (name + a single emoji icon) --
// these are the duty types (Singing, Translating, ...) a Member can be
// scheduled against on the Date tab; this dialog only manages the type
// itself, never a scheduled duty for a particular Member/date (that
// happens exclusively via AssignDutyDialog on the Date tab). Used by
// AdminOverviewView's Taxonomy tab -- pass an existing DutyType to edit,
// or a default-constructed DutyType() (id() < 0) for "new", mirroring
// TagEditDialog/CategoryEditDialog.
class DutyTypeEditDialog : public FramelessDialog
{
    Q_OBJECT

public:
    DutyTypeEditDialog(const DutyType &dutyType, QWidget *parent = nullptr);

    DutyType dutyType() const;

private slots:
    void saveClicked();

private:
    int m_id = -1;
    QLineEdit *m_nameEdit = nullptr;
    QLabel *m_nameError = nullptr;
    QLineEdit *m_iconEdit = nullptr;
};

#pragma once

#include "FramelessDialog.h"

#include "Models/Team.h"

class QLabel;
class QLineEdit;
class QPlainTextEdit;

// Add/edit a team: name + description. Used by AdminOverviewView's Taxonomy
// tab, mirroring CategoryEditDialog -- pass an existing Team to edit, or a
// default-constructed Team() for "add new". Members are put on a team from
// MemberEditDialog, not here.
class TeamEditDialog : public FramelessDialog
{
    Q_OBJECT

public:
    TeamEditDialog(const Team &team, QWidget *parent = nullptr);

    Team team() const;

private slots:
    void saveClicked();

private:
    int m_id = -1;
    QLineEdit *m_nameEdit = nullptr;
    QLabel *m_nameError = nullptr;
    QPlainTextEdit *m_descriptionEdit = nullptr;
};

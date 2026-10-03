#pragma once

#include "FramelessDialog.h"

#include "Models/AccessRights.h"

class QCheckBox;
class QLabel;
class QListWidget;
class QTableWidget;
class SuggestLineEdit;
class AccessController;
class DutyTypeController;
class TeamController;
class UserController;

// Settings > Access Rights (Admin only): type who in the "Who" field --
// everyone, a member, a team, or a duty, suggested as you type (or pick
// one already given rights from the list under it) -- and tick, per
// section, whether they can
// open it and create / update / delete there. Each tick is saved straight
// away. Ticking create/update/delete also ticks view; clearing view clears
// the rest. What a person ends up with is everything their rules allow
// added together (see AccessController::rightsFor).
class AccessRightsDialog : public FramelessDialog
{
    Q_OBJECT

public:
    AccessRightsDialog(
        AccessController *accessController,
        UserController *userController,
        TeamController *teamController,
        DutyTypeController *dutyTypeController,
        QWidget *parent = nullptr);

private:
    // Fills the Who suggestions (everyone, members, teams, duties).
    void buildChoices();
    // Shows and loads the rules of the choice with this key.
    void selectSubject(int key);
    void loadRules();
    void checkboxToggled(Section section, AccessAction action, bool checked);
    // Lists Everyone plus whoever has at least one right ticked.
    void rebuildSetList();
    // What ticking `action` in `section` lets someone do, for the tooltip.
    static QString actionMeaning(Section section, AccessAction action);
    static QString sectionName(Section section);

    AccessController *m_accessController = nullptr;
    UserController *m_userController = nullptr;
    TeamController *m_teamController = nullptr;
    DutyTypeController *m_dutyTypeController = nullptr;

    SuggestLineEdit *m_whoEdit = nullptr;
    QListWidget *m_setList = nullptr;
    // (key, text) for every choice; see the key encoding in the .cpp.
    QList<QPair<int, QString>> m_choices;
    QLabel *m_subjectTitle = nullptr;
    QLabel *m_subjectHint = nullptr;
    QTableWidget *m_grid = nullptr;
    QLabel *m_errorLabel = nullptr;
    // [row = section][column = action]; nullptr where a section has no such action.
    QHash<Section, QHash<AccessAction, QCheckBox *>> m_checks;
    AccessSubject m_subject = AccessSubject::Everyone;
    int m_subjectId = 0;
    bool m_loading = false;
};

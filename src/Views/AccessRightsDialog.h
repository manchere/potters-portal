#pragma once

#include "FramelessDialog.h"

#include "Models/AccessRights.h"

class QCheckBox;
class QLabel;
class QListWidget;
class QTableWidget;
class AccessController;
class DutyTypeController;
class TeamController;
class UserController;

// Settings > Access Rights (Admin only): pick who on the left -- everyone,
// a member, a team, or a duty -- and tick, per section, whether they can
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

private slots:
    void subjectChanged();

private:
    void buildSubjectList();
    void loadRules();
    void checkboxToggled(Section section, AccessAction action, bool checked);
    void markSubjectsWithRules();
    // What ticking `action` in `section` lets someone do, for the tooltip.
    static QString actionMeaning(Section section, AccessAction action);
    static QString sectionName(Section section);

    AccessController *m_accessController = nullptr;
    UserController *m_userController = nullptr;
    TeamController *m_teamController = nullptr;
    DutyTypeController *m_dutyTypeController = nullptr;

    QListWidget *m_subjects = nullptr;
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

#pragma once

#include <QVector>
#include <QWidget>

#include "Models/AccessRights.h"
#include "Models/Category.h"
#include "Models/DutyType.h"
#include "Models/Tag.h"
#include "Models/Team.h"
#include "Models/User.h"

class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QPushButton;
class QButtonGroup;
class TagController;
class CategoryController;
class DutyTypeController;
class TeamController;
class UserController;

// "Taxonomy" tab -- a single unified list that shows exactly one of
// Members / Teams / Tags / Categories / Duty Types at a time (toggled via
// the buttons at the top), with a search-by-name filter, "+ Add"
// buttons, double-click to edit, and one generic Delete button that acts
// on whichever kind is currently shown. Tag rows are colored with the
// tag's own color; Member rows show their color badge at the side.
//
// "+ Add Duty" creates a duty *type* (a duty name + icon,
// e.g. "Ushering" + an emoji) -- it does NOT schedule a Member against a
// role for a specific Sunday. Scheduling a Member happens exclusively on
// the Schedule tab ("+ Assign Duty" / "+ Add Member").
//
// "+ Add Team" creates a team; Members are put on one from the Team field
// of Add/Edit Member, and each Member row shows their team.
//
// Adding, editing and deleting (any list) follow the Taxonomy rights set in
// Settings > Access Rights. Two things stay Admin-only whatever those say:
// changing a Member's role (the "Make Admin" / "Remove Admin" button, with
// UserController::setAdminRole re-checking the acting account), and
// editing or deleting an Admin's own profile.
class AdminOverviewView : public QWidget
{
    Q_OBJECT

public:
    AdminOverviewView(
        TagController *tagController,
        CategoryController *categoryController,
        DutyTypeController *dutyTypeController,
        UserController *userController,
        TeamController *teamController,
        QWidget *parent = nullptr);

public slots:
    // Reloads Members/Tags/Categories/Duty Types from their
    // controllers and rebuilds whichever list is currently shown.
    void refresh();

    // What the signed-in person may do here (see the class comment).
    // userId is who's signed in (-1 when nobody is), passed to
    // UserController::setAdminRole for the check.
    void setAccess(const SectionAccess &access, bool isAdmin, int userId);

private slots:
    void kindButtonClicked();
    void searchTextChanged(const QString &text);
    void addMemberClicked();
    void addTeamClicked();
    void addTagClicked();
    void addCategoryClicked();
    void addDutyTypeClicked();
    void deleteClicked();
    void rowDoubleClicked(QListWidgetItem *item);
    void toggleAdminRoleClicked();

private:
    enum class Kind { Members, Teams, Tags, Categories, DutyTypes };

    void setKind(Kind kind);
    void rebuildList();
    void updateAddButtonVisibility();
    // Shows the role button only for Members in Admin mode, labelled for
    // the selected Member's current role.
    void updateRoleButton();

    TagController *m_tagController = nullptr;
    CategoryController *m_categoryController = nullptr;
    DutyTypeController *m_dutyTypeController = nullptr;
    UserController *m_userController = nullptr;
    TeamController *m_teamController = nullptr;
    // False when someone isn't allowed `action`, after telling them so.
    bool checkAllowed(AccessAction action);
    // Only an Admin may change an Admin's profile.
    bool canChangeMember(int memberId);

    SectionAccess m_access;
    bool m_isAdmin = false;
    int m_adminUserId = -1;
    Kind m_kind = Kind::Members;

    QVector<User> m_users;
    QVector<Team> m_teams;
    QVector<Tag> m_tags;
    QVector<Category> m_categories;
    QVector<DutyType> m_dutyTypes;

    QPushButton *m_membersToggle = nullptr;
    QPushButton *m_teamsToggle = nullptr;
    QPushButton *m_tagsToggle = nullptr;
    QPushButton *m_categoriesToggle = nullptr;
    QPushButton *m_dutyTypesToggle = nullptr;
    QButtonGroup *m_kindGroup = nullptr;

    QLineEdit *m_searchEdit = nullptr;

    QPushButton *m_addMemberButton = nullptr;
    QPushButton *m_addTeamButton = nullptr;
    QPushButton *m_addTagButton = nullptr;
    QPushButton *m_addCategoryButton = nullptr;
    QPushButton *m_addDutyTypeButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QPushButton *m_roleButton = nullptr;

    QListWidget *m_list = nullptr;
};

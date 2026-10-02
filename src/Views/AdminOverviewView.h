#pragma once

#include <QVector>
#include <QWidget>

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
// Adding/editing a Member or a Team is Admin-only (gated the same way the Schedule tab
// is); Tags, Categories, and Duty Types stay open to everyone, as
// before. So is changing a Member's role: the "Make Admin" / "Remove
// Admin" button only exists in Admin mode, and UserController::setAdminRole
// re-checks that the acting account is still an Admin.
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

    // Gates the Member add button, double-click-to-edit on Member rows, and
    // the Admin role toggle -- Tags/Categories/Duty Types management
    // stays open to everyone. adminUserId is the logged-in Admin (-1 when
    // logged out), passed to UserController::setAdminRole for the check.
    void setAdminMode(bool isAdmin, int adminUserId);

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

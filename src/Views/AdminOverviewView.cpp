#include "AdminOverviewView.h"

#include <QButtonGroup>
#include <QColor>
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "ActionBar.h"
#include "MemberBadge.h"
#include "CategoryEditDialog.h"
#include "Controllers/CategoryController.h"
#include "Controllers/DutyTypeController.h"
#include "Controllers/TagController.h"
#include "Controllers/TeamController.h"
#include "Controllers/UserController.h"
#include "MemberEditDialog.h"
#include "DutyTypeEditDialog.h"
#include "TagEditDialog.h"
#include "TeamEditDialog.h"

AdminOverviewView::AdminOverviewView(
    TagController *tagController,
    CategoryController *categoryController,
    DutyTypeController *dutyTypeController,
    UserController *userController,
    TeamController *teamController,
    QWidget *parent)
    : QWidget(parent)
    , m_tagController(tagController)
    , m_categoryController(categoryController)
    , m_dutyTypeController(dutyTypeController)
    , m_userController(userController)
    , m_teamController(teamController)
{
    auto *title = new QLabel(QStringLiteral("Taxonomy"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(
        QStringLiteral("Browse Members, Teams, Tags, Categories, or Duty Types one list at a time. "
                        "Double-click a row to edit it, or use Delete to remove it."),
        this);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));
    subtitle->setWordWrap(true);

    m_membersToggle = new QPushButton(QStringLiteral("Members"), this);
    m_teamsToggle = new QPushButton(QStringLiteral("Teams"), this);
    m_tagsToggle = new QPushButton(QStringLiteral("Tags"), this);
    m_categoriesToggle = new QPushButton(QStringLiteral("Categories"), this);
    m_dutyTypesToggle = new QPushButton(QStringLiteral("Duty Types"), this);
    for (QPushButton *toggle : {m_membersToggle, m_teamsToggle, m_tagsToggle, m_categoriesToggle, m_dutyTypesToggle}) {
        toggle->setCheckable(true);
        toggle->setObjectName(QStringLiteral("secondaryButton"));
        connect(toggle, &QPushButton::clicked, this, &AdminOverviewView::kindButtonClicked);
    }
    m_membersToggle->setChecked(true);
    m_kindGroup = new QButtonGroup(this);
    m_kindGroup->setExclusive(true);
    m_kindGroup->addButton(m_membersToggle);
    m_kindGroup->addButton(m_teamsToggle);
    m_kindGroup->addButton(m_tagsToggle);
    m_kindGroup->addButton(m_categoriesToggle);
    m_kindGroup->addButton(m_dutyTypesToggle);

    auto *toggleRow = new QHBoxLayout;
    toggleRow->addWidget(m_membersToggle);
    toggleRow->addWidget(m_teamsToggle);
    toggleRow->addWidget(m_tagsToggle);
    toggleRow->addWidget(m_categoriesToggle);
    toggleRow->addWidget(m_dutyTypesToggle);
    toggleRow->addStretch();

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("Search by name..."));
    connect(m_searchEdit, &QLineEdit::textChanged, this, &AdminOverviewView::searchTextChanged);

    m_addMemberButton = new QPushButton(QStringLiteral("+  Add Member"), this);
    connect(m_addMemberButton, &QPushButton::clicked, this, &AdminOverviewView::addMemberClicked);
    m_addTeamButton = new QPushButton(QStringLiteral("+  Add Team"), this);
    m_addTeamButton->setToolTip(QStringLiteral("Create a team -- put Members on it from Add/Edit Member"));
    connect(m_addTeamButton, &QPushButton::clicked, this, &AdminOverviewView::addTeamClicked);
    m_addTagButton = new QPushButton(QStringLiteral("+  Add Tag"), this);
    connect(m_addTagButton, &QPushButton::clicked, this, &AdminOverviewView::addTagClicked);
    m_addCategoryButton = new QPushButton(QStringLiteral("+  Add Category"), this);
    connect(m_addCategoryButton, &QPushButton::clicked, this, &AdminOverviewView::addCategoryClicked);
    m_addDutyTypeButton = new QPushButton(QStringLiteral("+  Add Duty Type"), this);
    m_addDutyTypeButton->setToolTip(
        QStringLiteral("Define a new duty type (name + icon) -- to give a Member a duty on a "
                        "specific Sunday, use the Schedule tab instead."));
    connect(m_addDutyTypeButton, &QPushButton::clicked, this, &AdminOverviewView::addDutyTypeClicked);
    for (QPushButton *addButton : {m_addMemberButton, m_addTeamButton, m_addTagButton, m_addCategoryButton, m_addDutyTypeButton}) {
        addButton->setObjectName(QStringLiteral("secondaryButton"));
    }


    m_list = new QListWidget(this);
    m_list->setAlternatingRowColors(true);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &AdminOverviewView::rowDoubleClicked);
    connect(m_list, &QListWidget::currentItemChanged, this, &AdminOverviewView::updateRoleButton);

    m_deleteButton = new QPushButton(QStringLiteral("Delete"), this);
    m_deleteButton->setObjectName(QStringLiteral("dangerButton"));
    connect(m_deleteButton, &QPushButton::clicked, this, &AdminOverviewView::deleteClicked);
    m_roleButton = new QPushButton(QStringLiteral("Make Admin"), this);
    m_roleButton->setObjectName(QStringLiteral("secondaryButton"));
    connect(m_roleButton, &QPushButton::clicked, this, &AdminOverviewView::toggleAdminRoleClicked);
    auto *actionBar = new ActionBar(this);
    actionBar->addWidget(m_addMemberButton);
    actionBar->addWidget(m_addTeamButton);
    actionBar->addWidget(m_addTagButton);
    actionBar->addWidget(m_addCategoryButton);
    actionBar->addWidget(m_addDutyTypeButton);
    actionBar->addSeparator();
    actionBar->addWidget(m_roleButton);
    actionBar->addStretch();
    actionBar->addWidget(m_deleteButton);

    auto *content = new QVBoxLayout;
    content->setSpacing(12);
    content->addWidget(title);
    content->addWidget(subtitle);
    content->addSpacing(6);
    content->addLayout(toggleRow);
    content->addWidget(m_searchEdit);
    content->addWidget(m_list, 1);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);
    layout->addLayout(content, 1);
    layout->addWidget(actionBar);

    updateAddButtonVisibility();
    refresh();
}

void AdminOverviewView::setAdminMode(bool isAdmin, int adminUserId)
{
    m_isAdmin = isAdmin;
    m_adminUserId = isAdmin ? adminUserId : -1;
    updateAddButtonVisibility();
    updateRoleButton();
}

void AdminOverviewView::updateRoleButton()
{
    const bool showForKind = m_isAdmin && m_kind == Kind::Members;
    m_roleButton->setVisible(showForKind);
    if (!showForKind) {
        return;
    }
    const QListWidgetItem *selected = m_list->currentItem();
    const int id = selected ? selected->data(Qt::UserRole).toInt() : -1;
    for (const User &user : m_users) {
        if (user.id() == id) {
            m_roleButton->setEnabled(true);
            m_roleButton->setText(user.isAdmin() ? QStringLiteral("Remove Admin") : QStringLiteral("Make Admin"));
            return;
        }
    }
    m_roleButton->setEnabled(false);
    m_roleButton->setText(QStringLiteral("Make Admin"));
}

void AdminOverviewView::toggleAdminRoleClicked()
{
    if (!m_isAdmin || m_kind != Kind::Members) {
        return;
    }
    const QListWidgetItem *selected = m_list->currentItem();
    if (!selected) {
        return;
    }
    const User target = m_userController->userById(selected->data(Qt::UserRole).toInt());
    if (target.id() < 0) {
        refresh();
        return;
    }
    const bool makeAdmin = !target.isAdmin();
    const QString title = makeAdmin ? QStringLiteral("Make Admin") : QStringLiteral("Remove Admin");
    const QString question = makeAdmin
        ? QStringLiteral("Make %1 an Admin?\n\nAdmins can unlock Admin mode on the desktop app with their "
                         "password and manage members, schedules, and songs.").arg(target.name())
        : QStringLiteral("Remove Admin from %1?\n\nThey'll stay a Member and keep their duties, "
                         "but can no longer unlock Admin mode.").arg(target.name());
    if (QMessageBox::question(this, title, question) != QMessageBox::Yes) {
        return;
    }
    if (!m_userController->setAdminRole(m_adminUserId, target.id(), makeAdmin)) {
        QMessageBox::warning(this, title, m_userController->lastError());
        return;
    }
    refresh();
}

void AdminOverviewView::updateAddButtonVisibility()
{
    // Members and Teams are a profile/login concern -- Admin-only, same as
    // the Date tab. Tags/Categories/Duty Types stay open to everyone.
    m_addMemberButton->setVisible(m_isAdmin);
    m_addTeamButton->setVisible(m_isAdmin);
}

void AdminOverviewView::kindButtonClicked()
{
    if (m_membersToggle->isChecked()) {
        setKind(Kind::Members);
    } else if (m_teamsToggle->isChecked()) {
        setKind(Kind::Teams);
    } else if (m_tagsToggle->isChecked()) {
        setKind(Kind::Tags);
    } else if (m_categoriesToggle->isChecked()) {
        setKind(Kind::Categories);
    } else {
        setKind(Kind::DutyTypes);
    }
}

void AdminOverviewView::setKind(Kind kind)
{
    m_kind = kind;
    rebuildList();
    updateRoleButton();
}

void AdminOverviewView::searchTextChanged(const QString &)
{
    rebuildList();
}

void AdminOverviewView::refresh()
{
    m_users = m_userController->allUsers();
    m_teams = m_teamController->allTeams();
    m_tags = m_tagController->allTags();
    m_categories = m_categoryController->allCategories();
    m_dutyTypes = m_dutyTypeController->allDutyTypes();
    rebuildList();
}

void AdminOverviewView::rebuildList()
{
    const QListWidgetItem *previous = m_list->currentItem();
    const int previousId = previous ? previous->data(Qt::UserRole).toInt() : -1;
    m_list->clear();
    const QString search = m_searchEdit->text().trimmed();

    if (m_kind == Kind::Members) {
        QHash<int, QString> teamNames;
        for (const Team &team : std::as_const(m_teams)) {
            teamNames.insert(team.id(), team.name());
        }
        for (const User &user : m_users) {
            if (!search.isEmpty() && !user.name().contains(search, Qt::CaseInsensitive)) {
                continue;
            }
            auto *item = new QListWidgetItem(m_list);
            item->setData(Qt::UserRole, user.id());
            item->setSizeHint(QSize(0, 48));
            m_list->addItem(item);

            auto *row = new QWidget(m_list);
            auto *rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(8, 4, 8, 4);
            rowLayout->setSpacing(10);
            rowLayout->addWidget(MemberBadge::make(user.name(), user.color(), 32, row));
            auto *textContainer = new QWidget(row);
            auto *textLayout = new QVBoxLayout(textContainer);
            textLayout->setContentsMargins(0, 0, 0, 0);
            textLayout->setSpacing(2);
            auto *nameLabel = new QLabel(user.name(), textContainer);
            nameLabel->setStyleSheet(QStringLiteral("font-weight: 600;"));
            auto *emailLabel = new QLabel(user.email(), textContainer);
            emailLabel->setObjectName(QStringLiteral("mutedLabel"));
            textLayout->addWidget(nameLabel);
            textLayout->addWidget(emailLabel);
            rowLayout->addWidget(textContainer, 1);
            if (teamNames.contains(user.teamId())) {
                auto *teamLabel = new QLabel(teamNames.value(user.teamId()), row);
                teamLabel->setObjectName(QStringLiteral("dutyPill"));
                teamLabel->setToolTip(QStringLiteral("Team"));
                rowLayout->addWidget(teamLabel);
            }
            if (user.isAdmin()) {
                auto *adminLabel = new QLabel(QStringLiteral("Admin"), row);
                adminLabel->setStyleSheet(QStringLiteral(
                    "background: #14335c; color: white; border-radius: 8px; padding: 2px 8px; font-weight: 600;"));
                rowLayout->addWidget(adminLabel);
            }
            m_list->setItemWidget(item, row);
        }
    } else if (m_kind == Kind::Teams) {
        QHash<int, int> memberCounts;
        for (const User &user : std::as_const(m_users)) {
            ++memberCounts[user.teamId()];
        }
        for (const Team &team : std::as_const(m_teams)) {
            if (!search.isEmpty() && !team.name().contains(search, Qt::CaseInsensitive)) {
                continue;
            }
            const int count = memberCounts.value(team.id());
            auto *item = new QListWidgetItem(
                QStringLiteral("%1   (%2 %3)").arg(team.name()).arg(count)
                    .arg(count == 1 ? QStringLiteral("member") : QStringLiteral("members")),
                m_list);
            item->setData(Qt::UserRole, team.id());
            if (!team.description().isEmpty()) {
                item->setToolTip(team.description());
            }
        }
    } else if (m_kind == Kind::Tags) {
        for (const Tag &tag : m_tags) {
            if (!search.isEmpty() && !tag.name().contains(search, Qt::CaseInsensitive)) {
                continue;
            }
            auto *item = new QListWidgetItem(tag.name(), m_list);
            item->setData(Qt::UserRole, tag.id());
            if (!tag.description().isEmpty()) {
                item->setToolTip(tag.description());
            }
            const QColor background(tag.color());
            item->setBackground(background);
            item->setForeground(background.lightness() < 140 ? QColor(Qt::white) : QColor(0x1f, 0x24, 0x30));
        }
    } else if (m_kind == Kind::Categories) {
        for (const Category &category : m_categories) {
            if (!search.isEmpty() && !category.name().contains(search, Qt::CaseInsensitive)) {
                continue;
            }
            auto *item = new QListWidgetItem(category.name(), m_list);
            item->setData(Qt::UserRole, category.id());
            if (!category.description().isEmpty()) {
                item->setToolTip(category.description());
            }
        }
    } else {
        for (const DutyType &dutyType : m_dutyTypes) {
            if (!search.isEmpty() && !dutyType.name().contains(search, Qt::CaseInsensitive)) {
                continue;
            }
            auto *item = new QListWidgetItem(dutyType.iconAndName(), m_list);
            item->setData(Qt::UserRole, dutyType.id());
        }
    }

    for (int i = 0; i < m_list->count(); ++i) {
        if (previousId >= 0 && m_list->item(i)->data(Qt::UserRole).toInt() == previousId) {
            m_list->setCurrentRow(i);
            break;
        }
    }
    updateRoleButton();
}

void AdminOverviewView::addMemberClicked()
{
    if (!m_isAdmin) {
        return;
    }
    MemberEditDialog dialog(User(), m_userController, m_teamController, this);
    if (dialog.exec() == QDialog::Accepted) {
        refresh();
    }
}

void AdminOverviewView::addTeamClicked()
{
    if (!m_isAdmin) {
        return;
    }
    TeamEditDialog dialog(Team(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    Team newTeam = dialog.team();
    if (m_teamController->addTeam(newTeam)) {
        refresh();
    } else {
        QMessageBox::critical(this, QStringLiteral("Add Team"), m_teamController->lastError());
    }
}

void AdminOverviewView::addTagClicked()
{
    TagEditDialog dialog(Tag(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    Tag newTag = dialog.tag();
    if (m_tagController->addTag(newTag)) {
        refresh();
    } else {
        QMessageBox::critical(this, QStringLiteral("Add Tag"), m_tagController->lastError());
    }
}

void AdminOverviewView::addCategoryClicked()
{
    CategoryEditDialog dialog(Category(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    Category newCategory = dialog.category();
    if (m_categoryController->addCategory(newCategory)) {
        refresh();
    } else {
        QMessageBox::critical(this, QStringLiteral("Add Category"), m_categoryController->lastError());
    }
}

void AdminOverviewView::addDutyTypeClicked()
{
    DutyTypeEditDialog dialog(DutyType(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    DutyType newDutyType = dialog.dutyType();
    if (m_dutyTypeController->addDutyType(newDutyType)) {
        refresh();
    } else {
        QMessageBox::critical(this, QStringLiteral("Add Duty Type"), m_dutyTypeController->lastError());
    }
}

void AdminOverviewView::deleteClicked()
{
    QListWidgetItem *selected = m_list->currentItem();
    if (!selected) {
        QMessageBox::information(this, QStringLiteral("Delete"), QStringLiteral("Select an item first."));
        return;
    }
    const int id = selected->data(Qt::UserRole).toInt();

    if (m_kind == Kind::Members) {
        if (!m_isAdmin) {
            return;
        }
        if (id == m_adminUserId) {
            QMessageBox::information(this, QStringLiteral("Delete Member"),
                QStringLiteral("You can't delete your own account while logged in as it."));
            return;
        }
        if (QMessageBox::question(this, QStringLiteral("Delete Member"), QStringLiteral("Delete this member?"))
            != QMessageBox::Yes) {
            return;
        }
        if (m_userController->removeUser(id)) {
            refresh();
        } else {
            QMessageBox::critical(this, QStringLiteral("Delete Member"), m_userController->lastError());
        }
    } else if (m_kind == Kind::Teams) {
        if (!m_isAdmin) {
            return;
        }
        if (QMessageBox::question(this, QStringLiteral("Delete Team"),
                QStringLiteral("Delete this team? Its members stay, just without a team."))
            != QMessageBox::Yes) {
            return;
        }
        if (m_teamController->removeTeam(id)) {
            refresh();
        } else {
            QMessageBox::critical(this, QStringLiteral("Delete Team"), m_teamController->lastError());
        }
    } else if (m_kind == Kind::Tags) {
        if (QMessageBox::question(this, QStringLiteral("Delete Tag"), QStringLiteral("Delete this tag?"))
            != QMessageBox::Yes) {
            return;
        }
        if (m_tagController->removeTag(id)) {
            refresh();
        } else {
            QMessageBox::critical(this, QStringLiteral("Delete Tag"), m_tagController->lastError());
        }
    } else if (m_kind == Kind::Categories) {
        if (QMessageBox::question(this, QStringLiteral("Delete Category"), QStringLiteral("Delete this category?"))
            != QMessageBox::Yes) {
            return;
        }
        if (m_categoryController->removeCategory(id)) {
            refresh();
        } else {
            QMessageBox::critical(this, QStringLiteral("Delete Category"), m_categoryController->lastError());
        }
    } else {
        if (QMessageBox::question(this, QStringLiteral("Delete Duty Type"), QStringLiteral("Delete this duty type?"))
            != QMessageBox::Yes) {
            return;
        }
        if (m_dutyTypeController->removeDutyType(id)) {
            refresh();
        } else {
            QMessageBox::critical(this, QStringLiteral("Delete Duty Type"),
                QStringLiteral("Couldn't delete -- it may still be used by one or more duties.\n\n%1")
                    .arg(m_dutyTypeController->lastError()));
        }
    }
}

void AdminOverviewView::rowDoubleClicked(QListWidgetItem *item)
{
    if (!item) {
        return;
    }
    const int id = item->data(Qt::UserRole).toInt();

    if (m_kind == Kind::Members) {
        if (!m_isAdmin) {
            QMessageBox::information(this, QStringLiteral("Edit Member"),
                QStringLiteral("Log in as Admin (lock icon in the title bar) to edit members."));
            return;
        }
        const User existing = m_userController->userById(id);
        if (existing.id() < 0) {
            return;
        }
        MemberEditDialog dialog(existing, m_userController, m_teamController, this);
        if (dialog.exec() == QDialog::Accepted) {
            refresh();
        }
    } else if (m_kind == Kind::Teams) {
        if (!m_isAdmin) {
            QMessageBox::information(this, QStringLiteral("Edit Team"),
                QStringLiteral("Log in as Admin (lock icon in the title bar) to edit teams."));
            return;
        }
        const Team existing = m_teamController->teamById(id);
        if (existing.id() < 0) {
            return;
        }
        TeamEditDialog dialog(existing, this);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        Team updated = dialog.team();
        if (m_teamController->updateTeam(updated)) {
            refresh();
        } else {
            QMessageBox::critical(this, QStringLiteral("Edit Team"), m_teamController->lastError());
        }
    } else if (m_kind == Kind::Tags) {
        const Tag existing = m_tagController->tagById(id);
        if (existing.id() < 0) {
            return;
        }
        TagEditDialog dialog(existing, this);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        Tag updated = dialog.tag();
        if (m_tagController->updateTag(updated)) {
            refresh();
        } else {
            QMessageBox::critical(this, QStringLiteral("Edit Tag"), m_tagController->lastError());
        }
    } else if (m_kind == Kind::Categories) {
        const Category existing = m_categoryController->categoryById(id);
        if (existing.id() < 0) {
            return;
        }
        CategoryEditDialog dialog(existing, this);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        Category updated = dialog.category();
        if (m_categoryController->updateCategory(updated)) {
            refresh();
        } else {
            QMessageBox::critical(this, QStringLiteral("Edit Category"), m_categoryController->lastError());
        }
    } else {
        const DutyType existing = m_dutyTypeController->dutyTypeById(id);
        if (existing.id() < 0) {
            return;
        }
        DutyTypeEditDialog dialog(existing, this);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        DutyType updated = dialog.dutyType();
        if (m_dutyTypeController->updateDutyType(updated)) {
            refresh();
        } else {
            QMessageBox::critical(this, QStringLiteral("Edit Duty Type"), m_dutyTypeController->lastError());
        }
    }
}

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
#include "ElidedLabel.h"
#include "Controllers/CategoryController.h"
#include "Controllers/DutyTypeController.h"
#include "Controllers/TagController.h"
#include "Controllers/TeamController.h"
#include "Controllers/UserController.h"
#include "MemberEditDialog.h"
#include "DutyTypeEditDialog.h"
#include "TagEditDialog.h"
#include "TeamEditDialog.h"
#include "MessageDialog.h"

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
    auto *title = new QLabel(tr("Taxonomy"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(
        tr("Browse Members, Teams, Tags, Categories, or Duty Types one list at a time. "
                        "Double-click a row to edit it, or use Delete to remove it."),
        this);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));
    subtitle->setWordWrap(true);

    m_membersToggle = new QPushButton(tr("Members"), this);
    m_teamsToggle = new QPushButton(tr("Teams"), this);
    m_tagsToggle = new QPushButton(tr("Tags"), this);
    m_categoriesToggle = new QPushButton(tr("Categories"), this);
    m_dutyTypesToggle = new QPushButton(tr("Duty Types"), this);
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
    m_searchEdit->setPlaceholderText(tr("Search by name..."));
    connect(m_searchEdit, &QLineEdit::textChanged, this, &AdminOverviewView::searchTextChanged);

    m_addMemberButton = new QPushButton(tr("+  Add Member"), this);
    connect(m_addMemberButton, &QPushButton::clicked, this, &AdminOverviewView::addMemberClicked);
    m_addTeamButton = new QPushButton(tr("+  Add Team"), this);
    m_addTeamButton->setToolTip(tr("Create a team -- put Members on it from Add/Edit Member"));
    connect(m_addTeamButton, &QPushButton::clicked, this, &AdminOverviewView::addTeamClicked);
    m_addTagButton = new QPushButton(tr("+  Add Tag"), this);
    connect(m_addTagButton, &QPushButton::clicked, this, &AdminOverviewView::addTagClicked);
    m_addCategoryButton = new QPushButton(tr("+  Add Category"), this);
    connect(m_addCategoryButton, &QPushButton::clicked, this, &AdminOverviewView::addCategoryClicked);
    m_addDutyTypeButton = new QPushButton(tr("+  Add Duty Type"), this);
    m_addDutyTypeButton->setToolTip(
        tr("Define a new duty type (name + icon) -- to give a Member a duty on a "
                        "specific Sunday, use the Schedule tab instead."));
    connect(m_addDutyTypeButton, &QPushButton::clicked, this, &AdminOverviewView::addDutyTypeClicked);
    for (QPushButton *addButton : {m_addMemberButton, m_addTeamButton, m_addTagButton, m_addCategoryButton, m_addDutyTypeButton}) {
        addButton->setObjectName(QStringLiteral("secondaryButton"));
    }


    m_list = new QListWidget(this);
    m_list->setAlternatingRowColors(true);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &AdminOverviewView::rowDoubleClicked);
    connect(m_list, &QListWidget::currentItemChanged, this, &AdminOverviewView::updateRoleButton);

    m_deleteButton = new QPushButton(tr("Delete"), this);
    m_deleteButton->setObjectName(QStringLiteral("dangerButton"));
    connect(m_deleteButton, &QPushButton::clicked, this, &AdminOverviewView::deleteClicked);
    m_roleButton = new QPushButton(tr("Make Admin"), this);
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

void AdminOverviewView::setAccess(const SectionAccess &access, bool isAdmin, int userId)
{
    m_access = access;
    m_isAdmin = isAdmin;
    m_adminUserId = userId;
    updateAddButtonVisibility();
    updateRoleButton();
}

bool AdminOverviewView::checkAllowed(AccessAction action)
{
    if (m_access.allows(action)) {
        return true;
    }
    MessageDialog::information(this, tr("Taxonomy"),
        tr("You don't have access to do that here. Ask an Admin if you need it."));
    return false;
}

bool AdminOverviewView::canChangeMember(int memberId)
{
    if (m_isAdmin) {
        return true;
    }
    for (const User &user : std::as_const(m_users)) {
        if (user.id() == memberId && user.isAdmin()) {
            MessageDialog::information(this, tr("Taxonomy"), tr("Only an Admin can change an Admin's profile."));
            return false;
        }
    }
    return true;
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
            m_roleButton->setText(user.isAdmin() ? tr("Remove Admin") : tr("Make Admin"));
            return;
        }
    }
    m_roleButton->setEnabled(false);
    m_roleButton->setText(tr("Make Admin"));
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
    const QString title = makeAdmin ? tr("Make Admin") : tr("Remove Admin");
    const QString question = makeAdmin
        ? tr("Make %1 an Admin?\n\nAdmins can unlock Admin mode on the desktop app with their "
                         "password and manage members, schedules, and songs.").arg(target.name())
        : tr("Remove Admin from %1?\n\nThey'll stay a Member and keep their duties, "
                         "but can no longer unlock Admin mode.").arg(target.name());
    if (MessageDialog::question(this, title, question) != QMessageBox::Yes) {
        return;
    }
    if (!m_userController->setAdminRole(m_adminUserId, target.id(), makeAdmin)) {
        MessageDialog::warning(this, title, m_userController->lastError());
        return;
    }
    refresh();
}

void AdminOverviewView::updateAddButtonVisibility()
{
    for (QPushButton *addButton : {m_addMemberButton, m_addTeamButton, m_addTagButton, m_addCategoryButton,
                                   m_addDutyTypeButton}) {
        addButton->setVisible(m_access.create);
    }
    m_deleteButton->setVisible(m_access.remove);
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
                teamLabel->setToolTip(tr("Team"));
                rowLayout->addWidget(teamLabel);
            }
            if (user.isAdmin()) {
                auto *adminLabel = new QLabel(tr("Admin"), row);
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
                    .arg(count == 1 ? tr("member") : tr("members")),
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
            auto *item = new QListWidgetItem(m_list);
            item->setData(Qt::UserRole, tag.id());
            if (!tag.description().isEmpty()) {
                item->setToolTip(tag.description());
            }

            // The tag as a rounded pill in its own color (like the duty
            // tags on the Schedule tab), its description beside it.
            auto *row = new QWidget(m_list);
            auto *rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(8, 4, 8, 4);
            rowLayout->setSpacing(12);
            const QColor background(tag.color());
            const QColor text = background.lightness() < 140 ? QColor(Qt::white) : QColor(0x1f, 0x24, 0x30);
            auto *pill = new QLabel(tag.name(), row);
            pill->setStyleSheet(QStringLiteral(
                "QLabel { background: %1; color: %2; border-radius: 12px; padding: 4px 12px; font-weight: 600; }")
                .arg(background.name(), text.name()));
            pill->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            rowLayout->addWidget(pill, 0, Qt::AlignVCenter);
            if (!tag.description().isEmpty()) {
                auto *description = new ElidedLabel(tag.description(), row);
                description->setObjectName(QStringLiteral("mutedLabel"));
                rowLayout->addWidget(description, 1, Qt::AlignVCenter);
            } else {
                rowLayout->addStretch(1);
            }
            row->ensurePolished();
            item->setSizeHint(QSize(0, std::max(40, row->sizeHint().height())));
            m_list->setItemWidget(item, row);
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
    if (!checkAllowed(AccessAction::Create)) {
        return;
    }
    MemberEditDialog dialog(User(), m_userController, m_teamController, this);
    if (dialog.exec() == QDialog::Accepted) {
        refresh();
    }
}

void AdminOverviewView::addTeamClicked()
{
    if (!checkAllowed(AccessAction::Create)) {
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
        MessageDialog::critical(this, tr("Add Team"), m_teamController->lastError());
    }
}

void AdminOverviewView::addTagClicked()
{
    if (!checkAllowed(AccessAction::Create)) {
        return;
    }
    TagEditDialog dialog(Tag(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    Tag newTag = dialog.tag();
    if (m_tagController->addTag(newTag)) {
        refresh();
    } else {
        MessageDialog::critical(this, tr("Add Tag"), m_tagController->lastError());
    }
}

void AdminOverviewView::addCategoryClicked()
{
    if (!checkAllowed(AccessAction::Create)) {
        return;
    }
    CategoryEditDialog dialog(Category(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    Category newCategory = dialog.category();
    if (m_categoryController->addCategory(newCategory)) {
        refresh();
    } else {
        MessageDialog::critical(this, tr("Add Category"), m_categoryController->lastError());
    }
}

void AdminOverviewView::addDutyTypeClicked()
{
    if (!checkAllowed(AccessAction::Create)) {
        return;
    }
    DutyTypeEditDialog dialog(DutyType(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    DutyType newDutyType = dialog.dutyType();
    if (m_dutyTypeController->addDutyType(newDutyType)) {
        refresh();
    } else {
        MessageDialog::critical(this, tr("Add Duty Type"), m_dutyTypeController->lastError());
    }
}

void AdminOverviewView::deleteClicked()
{
    QListWidgetItem *selected = m_list->currentItem();
    if (!selected) {
        MessageDialog::information(this, tr("Delete"), tr("Select an item first."));
        return;
    }
    const int id = selected->data(Qt::UserRole).toInt();
    if (!checkAllowed(AccessAction::Delete)) {
        return;
    }

    if (m_kind == Kind::Members) {
        if (!canChangeMember(id)) {
            return;
        }
        if (id == m_adminUserId) {
            MessageDialog::information(this, tr("Delete Member"),
                tr("You can't delete your own account while logged in as it."));
            return;
        }
        if (MessageDialog::question(this, tr("Delete Member"), tr("Delete this member?"))
            != QMessageBox::Yes) {
            return;
        }
        if (m_userController->removeUser(id)) {
            refresh();
        } else {
            MessageDialog::critical(this, tr("Delete Member"), m_userController->lastError());
        }
    } else if (m_kind == Kind::Teams) {
        if (MessageDialog::question(this, tr("Delete Team"),
                tr("Delete this team? Its members stay, just without a team."))
            != QMessageBox::Yes) {
            return;
        }
        if (m_teamController->removeTeam(id)) {
            refresh();
        } else {
            MessageDialog::critical(this, tr("Delete Team"), m_teamController->lastError());
        }
    } else if (m_kind == Kind::Tags) {
        if (MessageDialog::question(this, tr("Delete Tag"), tr("Delete this tag?"))
            != QMessageBox::Yes) {
            return;
        }
        if (m_tagController->removeTag(id)) {
            refresh();
        } else {
            MessageDialog::critical(this, tr("Delete Tag"), m_tagController->lastError());
        }
    } else if (m_kind == Kind::Categories) {
        if (MessageDialog::question(this, tr("Delete Category"), tr("Delete this category?"))
            != QMessageBox::Yes) {
            return;
        }
        if (m_categoryController->removeCategory(id)) {
            refresh();
        } else {
            MessageDialog::critical(this, tr("Delete Category"), m_categoryController->lastError());
        }
    } else {
        if (MessageDialog::question(this, tr("Delete Duty Type"), tr("Delete this duty type?"))
            != QMessageBox::Yes) {
            return;
        }
        if (m_dutyTypeController->removeDutyType(id)) {
            refresh();
        } else {
            MessageDialog::critical(this, tr("Delete Duty Type"),
                tr("Couldn't delete -- it may still be used by one or more duties.\n\n%1")
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
    if (!checkAllowed(AccessAction::Update)) {
        return;
    }

    if (m_kind == Kind::Members) {
        if (!canChangeMember(id)) {
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
            MessageDialog::critical(this, tr("Edit Team"), m_teamController->lastError());
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
            MessageDialog::critical(this, tr("Edit Tag"), m_tagController->lastError());
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
            MessageDialog::critical(this, tr("Edit Category"), m_categoryController->lastError());
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
            MessageDialog::critical(this, tr("Edit Duty Type"), m_dutyTypeController->lastError());
        }
    }
}

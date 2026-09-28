#include "AdminOverviewView.h"

#include <QButtonGroup>
#include <QColor>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "AvatarLoader.h"
#include "CategoryEditDialog.h"
#include "Controllers/CategoryController.h"
#include "Controllers/RoleTypeController.h"
#include "Controllers/TagController.h"
#include "Controllers/UserController.h"
#include "MemberEditDialog.h"
#include "RoleTypeEditDialog.h"
#include "TagEditDialog.h"

AdminOverviewView::AdminOverviewView(
    TagController *tagController,
    CategoryController *categoryController,
    RoleTypeController *roleTypeController,
    UserController *userController,
    QNetworkAccessManager *networkManager,
    QWidget *parent)
    : QWidget(parent)
    , m_tagController(tagController)
    , m_categoryController(categoryController)
    , m_roleTypeController(roleTypeController)
    , m_userController(userController)
    , m_networkManager(networkManager)
{
    auto *title = new QLabel(QStringLiteral("Taxonomy"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(
        QStringLiteral("Browse Members, Tags, Categories, or Assignment Types one list at a time. "
                        "Double-click a row to edit it, or use Delete to remove it."),
        this);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));

    m_membersToggle = new QPushButton(QStringLiteral("Members"), this);
    m_tagsToggle = new QPushButton(QStringLiteral("Tags"), this);
    m_categoriesToggle = new QPushButton(QStringLiteral("Categories"), this);
    m_assignmentTypesToggle = new QPushButton(QStringLiteral("Assignment Types"), this);
    for (QPushButton *toggle : {m_membersToggle, m_tagsToggle, m_categoriesToggle, m_assignmentTypesToggle}) {
        toggle->setCheckable(true);
        toggle->setObjectName(QStringLiteral("secondaryButton"));
        connect(toggle, &QPushButton::clicked, this, &AdminOverviewView::kindButtonClicked);
    }
    m_membersToggle->setChecked(true);
    m_kindGroup = new QButtonGroup(this);
    m_kindGroup->setExclusive(true);
    m_kindGroup->addButton(m_membersToggle);
    m_kindGroup->addButton(m_tagsToggle);
    m_kindGroup->addButton(m_categoriesToggle);
    m_kindGroup->addButton(m_assignmentTypesToggle);

    auto *toggleRow = new QHBoxLayout;
    toggleRow->addWidget(m_membersToggle);
    toggleRow->addWidget(m_tagsToggle);
    toggleRow->addWidget(m_categoriesToggle);
    toggleRow->addWidget(m_assignmentTypesToggle);
    toggleRow->addStretch();

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("Search by name..."));
    connect(m_searchEdit, &QLineEdit::textChanged, this, &AdminOverviewView::searchTextChanged);

    m_addMemberButton = new QPushButton(QStringLiteral("+  Add Member"), this);
    connect(m_addMemberButton, &QPushButton::clicked, this, &AdminOverviewView::addMemberClicked);
    m_addTagButton = new QPushButton(QStringLiteral("+  Add Tag"), this);
    connect(m_addTagButton, &QPushButton::clicked, this, &AdminOverviewView::addTagClicked);
    m_addCategoryButton = new QPushButton(QStringLiteral("+  Add Category"), this);
    connect(m_addCategoryButton, &QPushButton::clicked, this, &AdminOverviewView::addCategoryClicked);
    m_addAssignmentButton = new QPushButton(QStringLiteral("+  Add Assignment"), this);
    m_addAssignmentButton->setToolTip(
        QStringLiteral("Define a new duty type (name + icon) -- to schedule a Member against a role for a "
                        "specific Sunday, use the Date tab instead."));
    connect(m_addAssignmentButton, &QPushButton::clicked, this, &AdminOverviewView::addAssignmentTypeClicked);
    for (QPushButton *addButton : {m_addMemberButton, m_addTagButton, m_addCategoryButton, m_addAssignmentButton}) {
        addButton->setObjectName(QStringLiteral("secondaryButton"));
    }

    auto *addRow = new QHBoxLayout;
    addRow->addWidget(m_addMemberButton);
    addRow->addWidget(m_addTagButton);
    addRow->addWidget(m_addCategoryButton);
    addRow->addWidget(m_addAssignmentButton);
    addRow->addStretch();

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
    auto *bottomRow = new QHBoxLayout;
    bottomRow->addWidget(m_deleteButton);
    bottomRow->addWidget(m_roleButton);
    bottomRow->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(6);
    layout->addLayout(toggleRow);
    layout->addWidget(m_searchEdit);
    layout->addLayout(addRow);
    layout->addWidget(m_list, 1);
    layout->addLayout(bottomRow);

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
        : QStringLiteral("Remove Admin from %1?\n\nThey'll stay a Member and keep their assignments, "
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
    // Members are a profile/login concern -- Admin-only, same as the Date
    // tab. Tags/Categories/Assignment Types stay open to everyone.
    m_addMemberButton->setVisible(m_isAdmin);
}

void AdminOverviewView::kindButtonClicked()
{
    if (m_membersToggle->isChecked()) {
        setKind(Kind::Members);
    } else if (m_tagsToggle->isChecked()) {
        setKind(Kind::Tags);
    } else if (m_categoriesToggle->isChecked()) {
        setKind(Kind::Categories);
    } else {
        setKind(Kind::AssignmentTypes);
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
    m_tags = m_tagController->allTags();
    m_categories = m_categoryController->allCategories();
    m_roleTypes = m_roleTypeController->allRoleTypes();
    rebuildList();
}

void AdminOverviewView::rebuildList()
{
    const QListWidgetItem *previous = m_list->currentItem();
    const int previousId = previous ? previous->data(Qt::UserRole).toInt() : -1;
    m_list->clear();
    const QString search = m_searchEdit->text().trimmed();

    if (m_kind == Kind::Members) {
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
            auto *avatar = new QLabel(row);
            if (m_networkManager) {
                AvatarLoader::loadInto(*m_networkManager, user.avatarSeed(), avatar, 32);
            }
            rowLayout->addWidget(avatar);
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
            if (user.isAdmin()) {
                auto *adminLabel = new QLabel(QStringLiteral("Admin"), row);
                adminLabel->setStyleSheet(QStringLiteral(
                    "background: #14335c; color: white; border-radius: 8px; padding: 2px 8px; font-weight: 600;"));
                rowLayout->addWidget(adminLabel);
            }
            m_list->setItemWidget(item, row);
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
        for (const RoleType &roleType : m_roleTypes) {
            if (!search.isEmpty() && !roleType.name().contains(search, Qt::CaseInsensitive)) {
                continue;
            }
            auto *item = new QListWidgetItem(roleType.iconAndName(), m_list);
            item->setData(Qt::UserRole, roleType.id());
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
    MemberEditDialog dialog(User(), m_userController, m_networkManager, this);
    if (dialog.exec() == QDialog::Accepted) {
        refresh();
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

void AdminOverviewView::addAssignmentTypeClicked()
{
    RoleTypeEditDialog dialog(RoleType(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    RoleType newRoleType = dialog.roleType();
    if (m_roleTypeController->addRoleType(newRoleType)) {
        refresh();
    } else {
        QMessageBox::critical(this, QStringLiteral("Add Assignment Type"), m_roleTypeController->lastError());
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
        if (QMessageBox::question(this, QStringLiteral("Delete Assignment Type"), QStringLiteral("Delete this assignment type?"))
            != QMessageBox::Yes) {
            return;
        }
        if (m_roleTypeController->removeRoleType(id)) {
            refresh();
        } else {
            QMessageBox::critical(this, QStringLiteral("Delete Assignment Type"),
                QStringLiteral("Couldn't delete -- it may still be used by one or more assignments.\n\n%1")
                    .arg(m_roleTypeController->lastError()));
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
        MemberEditDialog dialog(existing, m_userController, m_networkManager, this);
        if (dialog.exec() == QDialog::Accepted) {
            refresh();
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
        const RoleType existing = m_roleTypeController->roleTypeById(id);
        if (existing.id() < 0) {
            return;
        }
        RoleTypeEditDialog dialog(existing, this);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        RoleType updated = dialog.roleType();
        if (m_roleTypeController->updateRoleType(updated)) {
            refresh();
        } else {
            QMessageBox::critical(this, QStringLiteral("Edit Assignment Type"), m_roleTypeController->lastError());
        }
    }
}

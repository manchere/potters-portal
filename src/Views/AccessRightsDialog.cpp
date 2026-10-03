#include "AccessRightsDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "Controllers/AccessController.h"
#include "Controllers/DutyTypeController.h"
#include "Controllers/TeamController.h"
#include "Controllers/UserController.h"
#include "SuggestLineEdit.h"

namespace
{
    // Each Who choice's key is kind * kKindStride + the record's id.
    constexpr int kKindStride = 100000000;

    int subjectKeyFor(AccessSubject subject, int id)
    {
        return static_cast<int>(subject) * kKindStride + (subject == AccessSubject::Everyone ? 0 : id);
    }

    const QVector<AccessAction> kActions{
        AccessAction::View, AccessAction::Create, AccessAction::Update, AccessAction::Delete};

    // "member:12" -- matches AccessController::subjectsWithRules.
    QString subjectTag(AccessSubject subject, int id)
    {
        return subjectKey(subject) + QLatin1Char(':') + QString::number(subject == AccessSubject::Everyone ? 0 : id);
    }
}

AccessRightsDialog::AccessRightsDialog(
    AccessController *accessController,
    UserController *userController,
    TeamController *teamController,
    DutyTypeController *dutyTypeController,
    QWidget *parent)
    : FramelessDialog(parent)
    , m_accessController(accessController)
    , m_userController(userController)
    , m_teamController(teamController)
    , m_dutyTypeController(dutyTypeController)
{
    setWindowTitle(tr("Access Rights"));
    auto *heading = new QLabel(tr("Access Rights"), this);
    heading->setObjectName(QStringLiteral("pageTitle"));
    auto *intro = new QLabel(
        tr("Type who in the Who field, then tick what they can open and change. Members get everything their own, "
           "their team's and everyone's rules allow, plus a duty's rules while they have that duty on the "
           "upcoming Sunday. Admins can always do everything. Changes are saved as you tick."),
        this);
    intro->setObjectName(QStringLiteral("pageSubtitle"));
    intro->setWordWrap(true);

    m_whoEdit = new SuggestLineEdit(this);
    m_whoEdit->setPlaceholderText(tr("Type a member, team or duty"));
    connect(m_whoEdit, &QLineEdit::textChanged, this, [this]() {
        const int key = m_whoEdit->currentId();
        if (key >= 0) {
            selectSubject(key);
        }
    });
    auto *setCaption = new QLabel(tr("Already set"), this);
    setCaption->setObjectName(QStringLiteral("dutyCaption"));
    m_setList = new QListWidget(this);
    connect(m_setList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (QListWidgetItem *item = row >= 0 ? m_setList->item(row) : nullptr) {
            m_whoEdit->setCurrentId(item->data(Qt::UserRole).toInt());
        }
    });
    auto *whoColumn = new QVBoxLayout;
    whoColumn->setSpacing(6);
    auto *whoLabel = new QLabel(tr("Who"), this);
    whoLabel->setStyleSheet(QStringLiteral("font-weight: 700;"));
    whoColumn->addWidget(whoLabel);
    whoColumn->addWidget(m_whoEdit);
    whoColumn->addSpacing(6);
    whoColumn->addWidget(setCaption);
    whoColumn->addWidget(m_setList, 1);
    auto *whoBox = new QWidget(this);
    whoBox->setLayout(whoColumn);
    whoBox->setFixedWidth(250);

    m_subjectTitle = new QLabel(this);
    m_subjectTitle->setStyleSheet(QStringLiteral("font-weight: 700;"));
    m_subjectHint = new QLabel(this);
    m_subjectHint->setObjectName(QStringLiteral("mutedLabel"));
    m_subjectHint->setWordWrap(true);

    const QVector<Section> sections = allSections();
    m_grid = new QTableWidget(sections.size(), kActions.size(), this);
    m_grid->setHorizontalHeaderLabels({tr("View"), tr("Create"), tr("Update"), tr("Delete")});
    QStringList rowNames;
    for (Section section : sections) {
        rowNames << sectionName(section);
    }
    m_grid->setVerticalHeaderLabels(rowNames);
    m_grid->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_grid->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_grid->verticalHeader()->setDefaultSectionSize(40);
    m_grid->setSelectionMode(QAbstractItemView::NoSelection);
    m_grid->setFocusPolicy(Qt::NoFocus);
    m_grid->setEditTriggers(QAbstractItemView::NoEditTriggers);
    for (int row = 0; row < sections.size(); ++row) {
        const Section section = sections[row];
        for (int column = 0; column < kActions.size(); ++column) {
            const AccessAction action = kActions[column];
            auto *cell = new QWidget(m_grid);
            auto *cellLayout = new QHBoxLayout(cell);
            cellLayout->setContentsMargins(0, 0, 0, 0);
            cellLayout->setAlignment(Qt::AlignCenter);
            if (sectionHasAction(section, action)) {
                auto *check = new QCheckBox(cell);
                check->setToolTip(actionMeaning(section, action));
                connect(check, &QCheckBox::toggled, this, [this, section, action](bool checked) {
                    checkboxToggled(section, action, checked);
                });
                cellLayout->addWidget(check);
                m_checks[section][action] = check;
            } else {
                auto *none = new QLabel(QStringLiteral("—"), cell);
                none->setObjectName(QStringLiteral("mutedLabel"));
                none->setToolTip(tr("Not used in this section"));
                cellLayout->addWidget(none);
            }
            m_grid->setCellWidget(row, column, cell);
        }
    }
    m_grid->setMinimumHeight(m_grid->horizontalHeader()->height() + sections.size() * 40 + 4);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName(QStringLiteral("fieldError"));
    m_errorLabel->setWordWrap(true);

    auto *right = new QVBoxLayout;
    right->addWidget(m_subjectTitle);
    right->addWidget(m_subjectHint);
    right->addWidget(m_grid);
    right->addWidget(m_errorLabel);
    right->addStretch();

    auto *columns = new QHBoxLayout;
    columns->setSpacing(16);
    columns->addWidget(whoBox);
    columns->addLayout(right, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);

    auto *layout = contentLayout();
    layout->addWidget(heading);
    layout->addWidget(intro);
    layout->addLayout(columns, 1);
    layout->addWidget(buttons);
    resize(860, 560);

    buildChoices();
    rebuildSetList();
    m_whoEdit->setCurrentId(subjectKeyFor(AccessSubject::Everyone, 0));
}

QString AccessRightsDialog::sectionName(Section section)
{
    switch (section) {
    case Section::Reports: return tr("Reports");
    case Section::Songs: return tr("Songs");
    case Section::Inventory: return tr("Inventory");
    case Section::Taxonomy: return tr("Taxonomy");
    case Section::Feedback: return tr("Feedback");
    case Section::Settings: return tr("Settings");
    }
    return QString();
}

QString AccessRightsDialog::actionMeaning(Section section, AccessAction action)
{
    if (action == AccessAction::View) {
        return tr("Open the %1 tab").arg(sectionName(section));
    }
    switch (section) {
    case Section::Reports:
        return tr("Save reports as a web page or spreadsheet");
    case Section::Songs:
        return action == AccessAction::Create ? tr("Add songs")
             : action == AccessAction::Update ? tr("Edit songs") : tr("Delete songs");
    case Section::Inventory:
        return action == AccessAction::Create ? tr("Add items")
             : action == AccessAction::Update ? tr("Edit items, add tags, change status") : tr("Delete items");
    case Section::Taxonomy:
        return action == AccessAction::Create ? tr("Add members, teams, tags, categories and duty types")
             : action == AccessAction::Update ? tr("Edit members, teams, tags, categories and duty types")
                                              : tr("Delete members, teams, tags, categories and duty types");
    case Section::Feedback:
        return action == AccessAction::Create ? tr("Send requests")
             : action == AccessAction::Update ? tr("Read everyone's requests and mark them done")
                                              : tr("Delete requests");
    case Section::Settings:
        return tr("Change the app's theme colors");
    }
    return QString();
}

void AccessRightsDialog::buildChoices()
{
    m_choices.clear();
    m_choices.append({subjectKeyFor(AccessSubject::Everyone, 0), tr("Everyone (signed in or not)")});
    for (const User &user : m_userController->allUsers()) {
        if (!user.isAdmin()) { // Admins can always do everything
            m_choices.append({subjectKeyFor(AccessSubject::Member, user.id()), QStringLiteral("\U0001F464 ") + user.name()});
        }
    }
    for (const Team &team : m_teamController->allTeams()) {
        m_choices.append({subjectKeyFor(AccessSubject::Team, team.id()), QStringLiteral("\U0001F465 ") + team.name()});
    }
    for (const DutyType &dutyType : m_dutyTypeController->allDutyTypes()) {
        m_choices.append({subjectKeyFor(AccessSubject::Duty, dutyType.id()), dutyType.iconAndName()});
    }
    m_whoEdit->setItems(m_choices);
}

void AccessRightsDialog::rebuildSetList()
{
    const QSet<QString> withRules = m_accessController->subjectsWithRules();
    const int currentKey = subjectKeyFor(m_subject, m_subjectId);
    m_setList->blockSignals(true);
    m_setList->clear();
    for (const auto &choice : std::as_const(m_choices)) {
        const auto subject = static_cast<AccessSubject>(choice.first / kKindStride);
        const int id = choice.first % kKindStride;
        if (subject != AccessSubject::Everyone && !withRules.contains(subjectTag(subject, id))) {
            continue;
        }
        auto *item = new QListWidgetItem(choice.second, m_setList);
        item->setData(Qt::UserRole, choice.first);
        if (choice.first == currentKey) {
            m_setList->setCurrentItem(item);
        }
    }
    m_setList->blockSignals(false);
}

void AccessRightsDialog::selectSubject(int key)
{
    m_subject = static_cast<AccessSubject>(key / kKindStride);
    m_subjectId = key % kKindStride;
    for (const auto &choice : std::as_const(m_choices)) {
        if (choice.first == key) {
            m_subjectTitle->setText(choice.second);
        }
    }
    switch (m_subject) {
    case AccessSubject::Everyone:
        m_subjectHint->setText(tr("Applies to anyone using the app, including people who haven't signed in."));
        break;
    case AccessSubject::Member:
        m_subjectHint->setText(tr("Applies to this member whenever they're signed in."));
        break;
    case AccessSubject::Team:
        m_subjectHint->setText(tr("Applies to every member of this team whenever they're signed in."));
        break;
    case AccessSubject::Duty:
        m_subjectHint->setText(tr("Applies to members while they have this duty on the upcoming Sunday, "
                                  "and stops once that Sunday has passed."));
        break;
    }
    // Keep the Already set list's highlight in step.
    m_setList->blockSignals(true);
    m_setList->clearSelection();
    m_setList->setCurrentRow(-1);
    for (int i = 0; i < m_setList->count(); ++i) {
        if (m_setList->item(i)->data(Qt::UserRole).toInt() == key) {
            m_setList->setCurrentRow(i);
        }
    }
    m_setList->blockSignals(false);
    m_errorLabel->clear();
    loadRules();
}

void AccessRightsDialog::loadRules()
{
    m_loading = true;
    const QHash<Section, SectionAccess> rules = m_accessController->rulesFor(m_subject, m_subjectId);
    for (Section section : allSections()) {
        const SectionAccess access = rules.value(section);
        for (auto it = m_checks[section].cbegin(); it != m_checks[section].cend(); ++it) {
            it.value()->setChecked(access.allows(it.key()));
        }
    }
    m_loading = false;
}

void AccessRightsDialog::checkboxToggled(Section section, AccessAction action, bool checked)
{
    if (m_loading) {
        return;
    }
    // Keep the row sensible: changing anything needs the tab open, and a
    // closed tab can't be changed in.
    m_loading = true;
    QHash<AccessAction, QCheckBox *> &row = m_checks[section];
    if (checked && action != AccessAction::View && row.value(AccessAction::View)) {
        row.value(AccessAction::View)->setChecked(true);
    }
    if (!checked && action == AccessAction::View) {
        for (QCheckBox *check : std::as_const(row)) {
            check->setChecked(false);
        }
    }
    m_loading = false;

    SectionAccess access;
    for (auto it = row.cbegin(); it != row.cend(); ++it) {
        access.set(it.key(), it.value()->isChecked());
    }
    if (!m_accessController->setRule(m_subject, m_subjectId, section, access)) {
        m_errorLabel->setText(tr("Couldn't save: %1").arg(m_accessController->lastError()));
        loadRules();
        return;
    }
    m_errorLabel->clear();
    rebuildSetList();
}

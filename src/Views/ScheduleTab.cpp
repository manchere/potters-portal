#include "ScheduleTab.h"

#include "Style.h"

#include <algorithm>

#include <QBrush>
#include <QCheckBox>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QShortcut>
#include <QToolButton>
#include <QVBoxLayout>

#include "AssignDutyDialog.h"
#include "ActionBar.h"
#include "AddToScheduleDialog.h"
#include "MemberBadge.h"
#include "Controllers/DutyController.h"
#include "Controllers/DutyTypeController.h"
#include "Controllers/UserController.h"
#include "MemberStatsDialog.h"
#include "Models/Duty.h"
#include "Models/DutyType.h"
#include "Models/User.h"

namespace
{
    // The list runs from the upcoming Sunday through the end of October
    // next year (past Sundays are looked up on the Reports tab).
    QDate sundayListEnd(const QDate &today)
    {
        return QDate(today.year() + 1, 10, 31);
    }

    QString formatSunday(const QDate &date)
    {
        // e.g. "Sun 6 Sep 2026", per SCHEDULING_FUNCTIONAL_REQUIREMENTS.md.
        return date.toString(QStringLiteral("ddd d MMM yyyy"));
    }

    // Every way someone might type a Sunday into the search box: the list's
    // own "Sun 6 Sep 2026", the spelled-out "Sunday 6 September 2026",
    // "06/09/2026" and ISO "2026-09-06".
    QString sundaySearchText(const QDate &date)
    {
        return QStringList{
            formatSunday(date),
            date.toString(QStringLiteral("dddd d MMMM yyyy")),
            date.toString(QStringLiteral("dd/MM/yyyy")),
            date.toString(Qt::ISODate),
        }.join(QLatin1Char(' ')).toLower();
    }
}

ScheduleTab::ScheduleTab(
    DutyController *dutyController,
    UserController *userController,
    DutyTypeController *dutyTypeController,
    QWidget *parent)
    : QWidget(parent)
    , m_dutyController(dutyController)
    , m_userController(userController)
    , m_dutyTypeController(dutyTypeController)
{
    auto *title = new QLabel(QStringLiteral("Schedule"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(
        QStringLiteral("Pick a Sunday to see who's serving, and assign duties for it. "
                        "Double-click a member to edit their duties and backup."),
        this);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));
    subtitle->setWordWrap(true);

    m_assignButton = new QPushButton(QStringLiteral("+  Assign Duty"), this);
    connect(m_assignButton, &QPushButton::clicked, this, &ScheduleTab::assignClicked);
    m_addMemberButton = new QPushButton(QStringLiteral("+  Add Member"), this);
    m_addMemberButton->setObjectName(QStringLiteral("secondaryButton"));
    m_addMemberButton->setToolTip(
        QStringLiteral("Put a member on this Sunday: their duties and a backup"));
    connect(m_addMemberButton, &QPushButton::clicked, this, &ScheduleTab::addMemberClicked);
    m_assignForMemberButton = new QPushButton(QStringLiteral("+  Assign Another Duty"), this);
    m_assignForMemberButton->setObjectName(QStringLiteral("secondaryButton"));
    m_assignForMemberButton->setToolTip(
        QStringLiteral("Give the selected Member another duty on this Sunday"));
    connect(m_assignForMemberButton, &QPushButton::clicked, this, &ScheduleTab::assignForSelectedMemberClicked);
    m_copyButton = new QPushButton(QStringLiteral("Copy Schedule"), this);
    m_copyButton->setObjectName(QStringLiteral("secondaryButton"));
    m_copyButton->setToolTip(QStringLiteral("Copy this Sunday's duties (Ctrl+C)"));
    connect(m_copyButton, &QPushButton::clicked, this, &ScheduleTab::copyScheduleClicked);
    m_pasteButton = new QPushButton(QStringLiteral("Paste Schedule"), this);
    m_pasteButton->setObjectName(QStringLiteral("secondaryButton"));
    m_pasteButton->setToolTip(QStringLiteral("Paste the copied duties onto this Sunday (Ctrl+V)"));
    connect(m_pasteButton, &QPushButton::clicked, this, &ScheduleTab::pasteScheduleClicked);
    m_copiedLabel = new QLabel(this);
    m_copiedLabel->setObjectName(QStringLiteral("accentLabel"));

    auto *copyShortcut = new QShortcut(QKeySequence::Copy, this);
    copyShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(copyShortcut, &QShortcut::activated, this, &ScheduleTab::copyScheduleClicked);
    auto *pasteShortcut = new QShortcut(QKeySequence::Paste, this);
    pasteShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(pasteShortcut, &QShortcut::activated, this, &ScheduleTab::pasteScheduleClicked);

    m_sundayList = new QListWidget(this);
    m_sundayList->setFixedWidth(200);
    connect(m_sundayList, &QListWidget::currentItemChanged, this, &ScheduleTab::sundaySelectionChanged);

    m_resultsList = new QListWidget(this);
    m_resultsList->setAlternatingRowColors(true);
    connect(m_resultsList, &QListWidget::currentRowChanged, this, [this](int row) {
        QListWidgetItem *item = row >= 0 ? m_resultsList->item(row) : nullptr;
        m_selectedDutyId = item ? item->data(Qt::UserRole).toInt() : -1;
        m_selectedMemberId = item && item->data(Qt::UserRole + 1).isValid()
            ? item->data(Qt::UserRole + 1).toInt() : -1;
        updateActionState();
    });
    connect(m_resultsList, &QListWidget::itemDoubleClicked, this, &ScheduleTab::memberDoubleClicked);

    m_combineCheck = new QCheckBox(QStringLiteral("Combine each member's duties into one row"), this);
    m_combineCheck->setToolTip(QStringLiteral(
        "Show one row per member with all their duties. Each duty keeps its own backup "
        "and can still be edited on its own."));
    connect(m_combineCheck, &QCheckBox::toggled, this, &ScheduleTab::rebuildResults);

    m_editButton = new QPushButton(QStringLiteral("Edit"), this);
    m_editButton->setObjectName(QStringLiteral("secondaryButton"));
    connect(m_editButton, &QPushButton::clicked, this, &ScheduleTab::editClicked);
    m_deleteButton = new QPushButton(QStringLiteral("Delete"), this);
    m_deleteButton->setObjectName(QStringLiteral("dangerButton"));
    connect(m_deleteButton, &QPushButton::clicked, this, &ScheduleTab::deleteClicked);

    auto *actionBar = new ActionBar(this);
    actionBar->addWidget(m_assignButton);
    actionBar->addWidget(m_assignForMemberButton);
    actionBar->addWidget(m_editButton);
    actionBar->addSeparator();
    actionBar->addWidget(m_addMemberButton);
    actionBar->addSeparator();
    actionBar->addWidget(m_copyButton);
    actionBar->addWidget(m_pasteButton);
    actionBar->addWidget(m_copiedLabel);
    actionBar->addStretch();
    actionBar->addWidget(m_deleteButton);

    m_pastNotice = new QLabel(QStringLiteral("This Sunday has passed, so its schedule is read-only."), this);
    m_pastNotice->setObjectName(QStringLiteral("accentLabel"));
    m_pastNotice->setWordWrap(true);
    m_pastNotice->hide();
    setAdminMode(false);

    auto *resultsLayout = new QVBoxLayout;
    resultsLayout->addWidget(m_pastNotice);
    resultsLayout->addWidget(m_combineCheck);
    resultsLayout->addWidget(m_resultsList);
    auto *resultsBox = new QGroupBox(QStringLiteral("Duties"), this);
    resultsBox->setLayout(resultsLayout);

    m_sundaySearch = new QLineEdit(this);
    m_sundaySearch->setFixedWidth(200);
    m_sundaySearch->setPlaceholderText(QStringLiteral("Search date, duty, name..."));
    m_sundaySearch->setToolTip(QStringLiteral(
        "Find Sundays by date (e.g. \"27 Sep\" or \"October\"), by duty, or by a member's first or last name"));
    m_sundaySearch->setClearButtonEnabled(true);
    connect(m_sundaySearch, &QLineEdit::textChanged, this, &ScheduleTab::sundaySearchChanged);

    m_noSundayMatchLabel = new QLabel(QStringLiteral("No Sundays match."), this);
    m_noSundayMatchLabel->setObjectName(QStringLiteral("mutedLabel"));
    m_noSundayMatchLabel->hide();

    auto *sundayBox = new QGroupBox(QStringLiteral("Sundays"), this);
    auto *sundayLayout = new QVBoxLayout;
    sundayLayout->addWidget(m_sundaySearch);
    sundayLayout->addWidget(m_noSundayMatchLabel);
    sundayLayout->addWidget(m_sundayList);
    sundayBox->setLayout(sundayLayout);

    auto *columns = new QHBoxLayout;
    columns->setSpacing(16);
    columns->addWidget(sundayBox);
    columns->addWidget(resultsBox, 1);

    auto *content = new QVBoxLayout;
    content->setSpacing(12);
    content->addWidget(title);
    content->addWidget(subtitle);
    content->addSpacing(6);
    content->addLayout(columns);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);
    layout->addLayout(content, 1);
    layout->addWidget(actionBar);

    populateSundayList();
    selectSunday(nearestSunday(QDate::currentDate()));
}

namespace
{
    // The row's three columns (member, duty, backup) take fixed shares of
    // the width, so they line up from one row to the next.
    void addColumn(QHBoxLayout *layout, QWidget *column, int share)
    {
        QSizePolicy policy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        policy.setHorizontalStretch(share);
        column->setSizePolicy(policy);
        layout->addWidget(column);
    }
}

QWidget *ScheduleTab::buildBackupLine(const Duty &duty, QWidget *parent)
{
    auto *line = new QWidget(parent);
    auto *layout = new QHBoxLayout(line);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    const User backup = duty.supportMemberId() > 0 ? m_userController->userById(duty.supportMemberId()) : User();
    if (backup.id() >= 0) {
        layout->addWidget(MemberBadge::make(backup.name(), backup.color(), 20, line));
        auto *backupName = new QLabel(backup.name(), line);
        backupName->setObjectName(QStringLiteral("dutyBackupName"));
        backupName->setToolTip(backup.name());
        layout->addWidget(backupName, 1);
    } else {
        auto *none = new QLabel(QStringLiteral("—"), line);
        none->setObjectName(QStringLiteral("mutedLabel"));
        layout->addWidget(none, 1);
    }
    return line;
}

// One duty, laid out across the full row: the member (badge, name in
// bold, any notes beneath), the duty as an icon pill, and the backup
// with their own small badge.
QWidget *ScheduleTab::buildRow(const Duty &duty)
{
    auto *row = new QWidget(m_resultsList);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(10, 6, 12, 6);
    layout->setSpacing(12);

    const User member = duty.memberId() > 0 ? m_userController->userById(duty.memberId()) : User();
    if (member.id() >= 0) {
        layout->addWidget(MemberBadge::make(member.name(), member.color(), 32, row));
    } else {
        // Unfilled duty (or a deleted Member): an empty grey circle.
        auto *placeholder = new QLabel(row);
        placeholder->setFixedSize(32, 32);
        placeholder->setObjectName(QStringLiteral("avatarPlaceholder"));
        layout->addWidget(placeholder);
    }

    // --- Member: name, then notes if any.
    auto *memberColumn = new QWidget(row);
    auto *memberLayout = new QVBoxLayout(memberColumn);
    memberLayout->setContentsMargins(0, 0, 0, 0);
    memberLayout->setSpacing(1);
    auto *nameLabel = new QLabel(member.id() >= 0 ? member.name() : QStringLiteral("Nobody assigned"), memberColumn);
    nameLabel->setObjectName(member.id() >= 0 ? QStringLiteral("dutyMemberName") : QStringLiteral("dutyUnfilled"));
    nameLabel->setToolTip(nameLabel->text());
    memberLayout->addWidget(nameLabel);
    if (!duty.notes().isEmpty()) {
        auto *notesLabel = new QLabel(duty.notes(), memberColumn);
        notesLabel->setObjectName(QStringLiteral("mutedLabel"));
        notesLabel->setToolTip(duty.notes());
        memberLayout->addWidget(notesLabel);
    }
    addColumn(layout, memberColumn, 5);

    // --- Duty: icon + name in a pill.
    const DutyType dutyType = m_dutyTypeController->dutyTypeById(duty.dutyTypeId());
    auto *dutyColumn = new QWidget(row);
    auto *dutyLayout = new QHBoxLayout(dutyColumn);
    dutyLayout->setContentsMargins(0, 0, 0, 0);
    auto *dutyPill = new QLabel(dutyType.iconAndName(), dutyColumn);
    dutyPill->setObjectName(QStringLiteral("dutyPill"));
    dutyLayout->addWidget(dutyPill);
    dutyLayout->addStretch();
    addColumn(layout, dutyColumn, 4);

    // --- Backup: caption over a small badge + name, or a dash.
    auto *backupColumn = new QWidget(row);
    auto *backupLayout = new QVBoxLayout(backupColumn);
    backupLayout->setContentsMargins(0, 0, 0, 0);
    backupLayout->setSpacing(2);
    auto *backupCaption = new QLabel(QStringLiteral("Backup"), backupColumn);
    backupCaption->setObjectName(QStringLiteral("dutyCaption"));
    backupLayout->addWidget(backupCaption);
    backupLayout->addWidget(buildBackupLine(duty, backupColumn));
    addColumn(layout, backupColumn, 4);

    return row;
}

// One member with all their duties on this Sunday: badge and name (with
// any notes) on the left, then a line per duty -- its pill, its own
// backup, and an Edit button for just that duty.
QWidget *ScheduleTab::buildMemberRow(const User &member, const QVector<Duty> &duties)
{
    auto *row = new QWidget(m_resultsList);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(10, 6, 12, 6);
    layout->setSpacing(12);

    auto *badge = MemberBadge::make(member.name(), member.color(), 32, row);
    layout->addWidget(badge, 0, Qt::AlignTop);

    auto *memberColumn = new QWidget(row);
    auto *memberLayout = new QVBoxLayout(memberColumn);
    memberLayout->setContentsMargins(0, 0, 0, 0);
    memberLayout->setSpacing(1);
    auto *nameLabel = new QLabel(member.name(), memberColumn);
    nameLabel->setObjectName(QStringLiteral("dutyMemberName"));
    nameLabel->setToolTip(member.name());
    memberLayout->addWidget(nameLabel);
    QStringList notes;
    for (const Duty &duty : duties) {
        if (!duty.notes().isEmpty() && !notes.contains(duty.notes())) {
            notes.append(duty.notes());
        }
    }
    if (!notes.isEmpty()) {
        auto *notesLabel = new QLabel(notes.join(QStringLiteral(" · ")), memberColumn);
        notesLabel->setObjectName(QStringLiteral("mutedLabel"));
        notesLabel->setToolTip(notes.join(QLatin1Char('\n')));
        memberLayout->addWidget(notesLabel);
    }
    memberLayout->addStretch();
    addColumn(layout, memberColumn, 5);

    const bool canEdit = m_isAdmin && selectedSundayEditable();
    auto *dutiesColumn = new QWidget(row);
    auto *dutiesLayout = new QVBoxLayout(dutiesColumn);
    dutiesLayout->setContentsMargins(0, 0, 0, 0);
    dutiesLayout->setSpacing(4);
    for (const Duty &duty : duties) {
        auto *line = new QWidget(dutiesColumn);
        auto *lineLayout = new QHBoxLayout(line);
        lineLayout->setContentsMargins(0, 0, 0, 0);
        lineLayout->setSpacing(12);

        auto *dutyCell = new QWidget(line);
        auto *dutyCellLayout = new QHBoxLayout(dutyCell);
        dutyCellLayout->setContentsMargins(0, 0, 0, 0);
        auto *dutyPill = new QLabel(m_dutyTypeController->dutyTypeById(duty.dutyTypeId()).iconAndName(), dutyCell);
        dutyPill->setObjectName(QStringLiteral("dutyPill"));
        dutyCellLayout->addWidget(dutyPill);
        dutyCellLayout->addStretch();
        addColumn(lineLayout, dutyCell, 4);

        auto *backupCell = new QWidget(line);
        auto *backupCellLayout = new QHBoxLayout(backupCell);
        backupCellLayout->setContentsMargins(0, 0, 0, 0);
        backupCellLayout->setSpacing(6);
        auto *backupCaption = new QLabel(QStringLiteral("Backup"), backupCell);
        backupCaption->setObjectName(QStringLiteral("dutyCaption"));
        backupCellLayout->addWidget(backupCaption);
        backupCellLayout->addWidget(buildBackupLine(duty, backupCell), 1);
        addColumn(lineLayout, backupCell, 4);

        if (canEdit) {
            auto *editButton = new QToolButton(line);
            editButton->setObjectName(QStringLiteral("dutyEditButton"));
            editButton->setText(QStringLiteral("Edit"));
            editButton->setToolTip(QStringLiteral("Edit just this duty and its backup"));
            editButton->setCursor(Qt::PointingHandCursor);
            const int dutyId = duty.id();
            connect(editButton, &QToolButton::clicked, this, [this, dutyId] { editDuty(dutyId); });
            lineLayout->addWidget(editButton);
        }
        dutiesLayout->addWidget(line);
    }
    addColumn(layout, dutiesColumn, 8);

    return row;
}

void ScheduleTab::setAdminMode(bool isAdmin)
{
    m_isAdmin = isAdmin;
    m_assignButton->setVisible(isAdmin);
    m_addMemberButton->setVisible(isAdmin);
    m_assignForMemberButton->setVisible(isAdmin);
    m_editButton->setVisible(isAdmin);
    m_deleteButton->setVisible(isAdmin);
    m_copyButton->setVisible(isAdmin);
    m_pasteButton->setVisible(isAdmin);
    m_copiedLabel->setVisible(isAdmin);
    updateActionState();
    // Combined rows carry per-duty Edit buttons only an Admin sees.
    if (m_combineCheck->isChecked() && m_selectedDate.isValid()) {
        rebuildResults();
    }
}

bool ScheduleTab::selectedSundayEditable() const
{
    return DutyController::isEditableDate(m_selectedDate);
}

void ScheduleTab::updateActionState()
{
    const bool editable = m_isAdmin && selectedSundayEditable();
    const bool hasDuty = m_selectedDutyId >= 0;
    m_assignButton->setEnabled(editable);
    m_addMemberButton->setEnabled(editable);
    m_assignForMemberButton->setEnabled(editable && hasDuty);
    m_editButton->setEnabled(editable && hasDuty);
    m_deleteButton->setEnabled(editable && hasDuty);
    // On a combined row they act on the member's whole place this Sunday.
    const bool memberRow = m_selectedMemberId > 0;
    m_editButton->setText(memberRow ? QStringLiteral("Edit Member") : QStringLiteral("Edit"));
    m_deleteButton->setText(memberRow ? QStringLiteral("Remove Member") : QStringLiteral("Delete"));
    m_pastNotice->setVisible(m_selectedDate.isValid() && !selectedSundayEditable());
    updateCopyPasteState();
}

void ScheduleTab::updateCopyPasteState()
{
    m_copyButton->setEnabled(m_isAdmin && m_datesWithDuties.contains(m_selectedDate));
    m_pasteButton->setEnabled(m_isAdmin && selectedSundayEditable()
                              && m_copiedDate.isValid() && m_copiedDate != m_selectedDate);
    m_copiedLabel->setText(m_copiedDate.isValid()
        ? QStringLiteral("Copied: %1").arg(formatSunday(m_copiedDate))
        : QString());
}

void ScheduleTab::copyScheduleClicked()
{
    if (!m_isAdmin || !m_datesWithDuties.contains(m_selectedDate)) {
        return;
    }
    m_copiedDate = m_selectedDate;
    updateCopyPasteState();
}

void ScheduleTab::pasteScheduleClicked()
{
    if (!m_isAdmin || !selectedSundayEditable() || !m_copiedDate.isValid() || m_copiedDate == m_selectedDate) {
        return;
    }
    const QDate fromDate = m_copiedDate;
    const QDate toDate = m_selectedDate;
    const int sourceCount = m_dutyController->dutiesForDate(fromDate).size();
    if (sourceCount == 0) {
        QMessageBox::information(this, QStringLiteral("Paste Schedule"),
            QStringLiteral("%1 no longer has any duties to copy.").arg(formatSunday(fromDate)));
        m_copiedDate = QDate();
        updateCopyPasteState();
        return;
    }
    const int existingCount = m_dutyController->dutiesForDate(toDate).size();

    bool replaceExisting = false;
    if (existingCount == 0) {
        const QString question = QStringLiteral("Copy %1 duties from %2 onto %3?")
            .arg(sourceCount).arg(formatSunday(fromDate), formatSunday(toDate));
        if (QMessageBox::question(this, QStringLiteral("Paste Schedule"), question) != QMessageBox::Yes) {
            return;
        }
    } else {
        QMessageBox box(QMessageBox::Question, QStringLiteral("Paste Schedule"),
            QStringLiteral("%1 already has %2 duties.").arg(formatSunday(toDate)).arg(existingCount),
            QMessageBox::NoButton, this);
        box.setInformativeText(QStringLiteral(
            "Add to them: keeps what's there and adds the copied duties (skipping any duty the same "
            "member already has).\n\nReplace them: deletes this Sunday's duties, including any "
            "time-off requests members sent for them, then pastes the copied schedule."));
        QPushButton *addButton = box.addButton(QStringLiteral("Add to Them"), QMessageBox::AcceptRole);
        QPushButton *replaceButton = box.addButton(QStringLiteral("Replace Them"), QMessageBox::DestructiveRole);
        box.addButton(QMessageBox::Cancel);
        box.setDefaultButton(addButton);
        box.exec();
        if (box.clickedButton() == replaceButton) {
            replaceExisting = true;
        } else if (box.clickedButton() != addButton) {
            return;
        }
    }

    int copied = 0;
    int skipped = 0;
    if (!m_dutyController->copySchedule(fromDate, toDate, replaceExisting, &copied, &skipped)) {
        QMessageBox::critical(this, QStringLiteral("Paste Schedule"), m_dutyController->lastError());
        return;
    }
    populateSundayList();
    selectSunday(toDate);

    QString summary = QStringLiteral("Pasted %1 duties onto %2.").arg(copied).arg(formatSunday(toDate));
    if (skipped > 0) {
        summary += QStringLiteral("\nSkipped %1 already on that Sunday.").arg(skipped);
    }
    const QStringList unavailable = m_dutyController->membersMarkedUnavailable(toDate);
    if (!unavailable.isEmpty()) {
        summary += QStringLiteral("\n\nHeads up: these members marked themselves unavailable that day:\n  %1")
            .arg(unavailable.join(QStringLiteral("\n  ")));
    }
    QMessageBox::information(this, QStringLiteral("Paste Schedule"), summary);
}

QDate ScheduleTab::nearestSunday(const QDate &date)
{
    // QDate::dayOfWeek(): 1 = Monday ... 7 = Sunday.
    return date.addDays(7 - date.dayOfWeek());
}

void ScheduleTab::populateSundayList()
{
    m_sundayList->clear();

    // One query each for members and duties up front, rather than a
    // userById/dutyTypeById round trip per duty.
    QHash<int, QString> memberNames;
    for (const User &user : m_userController->allUsers()) {
        memberNames.insert(user.id(), user.name());
    }
    QHash<int, QString> dutyTypeNames;
    for (const DutyType &dutyType : m_dutyTypeController->allDutyTypes()) {
        dutyTypeNames.insert(dutyType.id(), dutyType.name());
    }

    m_datesWithDuties.clear();
    m_sundaySearchText.clear();
    for (const Duty &duty : m_dutyController->allDuties()) {
        const QDate date = duty.serviceDate();
        m_datesWithDuties.insert(date);
        m_sundaySearchText[date].append(QStringList{
            sundaySearchText(date),
            dutyTypeNames.value(duty.dutyTypeId()),
            memberNames.value(duty.memberId()),
            memberNames.value(duty.supportMemberId()),
        }.join(QLatin1Char(' ')).toLower());
    }

    const QDate today = QDate::currentDate();
    const QDate end = sundayListEnd(today);
    for (QDate sunday = nearestSunday(today); sunday <= end; sunday = sunday.addDays(7)) {
        auto *item = new QListWidgetItem(formatSunday(sunday), m_sundayList);
        item->setData(Qt::UserRole, sunday);
        applySundayItemStyle(item, false);
        if (!m_sundaySearchText.contains(sunday)) {
            m_sundaySearchText.insert(sunday, {sundaySearchText(sunday)});
        }
    }
    applySundayFilter();
}

void ScheduleTab::applySundayFilter()
{
    const QStringList terms = m_sundaySearch->text().toLower().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    int visibleCount = 0;
    for (int i = 0; i < m_sundayList->count(); ++i) {
        QListWidgetItem *item = m_sundayList->item(i);
        bool matches = terms.isEmpty();
        for (const QString &text : m_sundaySearchText.value(item->data(Qt::UserRole).toDate())) {
            matches = matches || std::all_of(terms.cbegin(), terms.cend(),
                [&text](const QString &term) { return text.contains(term); });
        }
        item->setHidden(!matches);
        if (matches) {
            ++visibleCount;
        }
    }
    m_noSundayMatchLabel->setVisible(visibleCount == 0);
}

void ScheduleTab::sundaySearchChanged()
{
    applySundayFilter();

    // Keep the Duties panel in step with what's listed: if the selected
    // Sunday just got filtered out, jump to the first one that matches.
    QListWidgetItem *current = m_sundayList->currentItem();
    if (!current || current->isHidden()) {
        for (int i = 0; i < m_sundayList->count(); ++i) {
            if (!m_sundayList->item(i)->isHidden()) {
                m_sundayList->setCurrentRow(i);
                current = m_sundayList->item(i);
                break;
            }
        }
    }
    if (current && !current->isHidden()) {
        m_sundayList->scrollToItem(current, QAbstractItemView::PositionAtCenter);
    }
}

// Selection uses an explicit background/foreground override (applied in
// sundaySelectionChanged) rather than relying on the list's normal
// selection highlight, since a per-item color already set for the "has
// duties" tint would otherwise compete with -- and often hide --
// the native selection styling.
void ScheduleTab::applySundayItemStyle(QListWidgetItem *item, bool isSelected) const
{
    const QDate date = item->data(Qt::UserRole).toDate();
    const bool hasDuties = m_datesWithDuties.contains(date);

    QFont font = item->font();
    font.setBold(hasDuties || isSelected);
    item->setFont(font);

    const Theme theme = currentTheme();
    const bool isDark = isDarkTheme(theme);
    if (isSelected) {
        // Navy & Gold selects in gold (navy text), like its buttons.
        if (theme == Theme::Navy) {
            item->setBackground(QColor(0xd4, 0xa7, 0x2c));
            item->setForeground(QColor(0x0b, 0x1a, 0x33));
        } else {
            item->setBackground(isDark ? QColor(0x1f, 0x4a, 0x85) : QColor(0x14, 0x33, 0x5c));
            item->setForeground(QColor(Qt::white));
        }
    } else if (hasDuties) {
        item->setBackground(isDark ? QColor(0x2a, 0x22, 0x10) : QColor(0xfa, 0xf3, 0xe0));
        item->setForeground(isDark ? QColor(0xe0, 0xb8, 0x5a) : QColor(0x8a, 0x6a, 0x1a));
    } else {
        item->setBackground(QBrush());
        item->setForeground(QBrush());
    }
}

void ScheduleTab::restyleSundayItems()
{
    for (int i = 0; i < m_sundayList->count(); ++i) {
        QListWidgetItem *item = m_sundayList->item(i);
        applySundayItemStyle(item, item->data(Qt::UserRole).toDate() == m_selectedDate);
    }
}

void ScheduleTab::selectSunday(const QDate &date)
{
    for (int i = 0; i < m_sundayList->count(); ++i) {
        if (m_sundayList->item(i)->data(Qt::UserRole).toDate() == date) {
            m_sundayList->setCurrentRow(i);
            m_sundayList->scrollToItem(m_sundayList->item(i), QAbstractItemView::PositionAtCenter);
            return;
        }
    }
    if (m_sundayList->count() > 0) {
        m_sundayList->setCurrentRow(0);
    }
}

void ScheduleTab::sundaySelectionChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (previous) {
        applySundayItemStyle(previous, false);
    }
    if (!current) {
        return;
    }
    applySundayItemStyle(current, true);
    m_selectedDate = current->data(Qt::UserRole).toDate();
    rebuildResults();
    updateActionState();
}

void ScheduleTab::rebuildResults()
{
    m_resultsList->clear();
    m_selectedDutyId = -1;
    m_selectedMemberId = -1;
    updateActionState();

    const QVector<Duty> duties = m_dutyController->dutiesForDate(m_selectedDate);
    if (duties.isEmpty()) {
        auto *item = new QListWidgetItem(m_resultsList);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        m_resultsList->addItem(item);
        m_resultsList->setItemWidget(item, new QLabel(QStringLiteral("No duties for this date."), m_resultsList));
        return;
    }
    if (!m_combineCheck->isChecked()) {
        for (const Duty &duty : duties) {
            auto *item = new QListWidgetItem(m_resultsList);
            item->setData(Qt::UserRole, duty.id());
            item->setSizeHint(QSize(0, 56));
            m_resultsList->addItem(item);
            m_resultsList->setItemWidget(item, buildRow(duty));
        }
        return;
    }

    // Combined: a row per member, in the order they first appear. Unfilled
    // duties (or ones whose member was deleted) stay as their own rows.
    QVector<int> memberOrder;
    QHash<int, QVector<Duty>> byMember;
    for (const Duty &duty : duties) {
        const int key = duty.memberId() > 0 && m_userController->userById(duty.memberId()).id() >= 0
            ? duty.memberId() : -duty.id() - 1;
        if (!byMember.contains(key)) {
            memberOrder.append(key);
        }
        byMember[key].append(duty);
    }
    for (int key : std::as_const(memberOrder)) {
        const QVector<Duty> &memberDuties = byMember[key];
        auto *item = new QListWidgetItem(m_resultsList);
        item->setData(Qt::UserRole, memberDuties.first().id());
        m_resultsList->addItem(item);
        if (key < 0) {
            item->setSizeHint(QSize(0, 56));
            m_resultsList->setItemWidget(item, buildRow(memberDuties.first()));
            continue;
        }
        item->setData(Qt::UserRole + 1, key);
        item->setSizeHint(QSize(0, std::max(56, 16 + static_cast<int>(memberDuties.size()) * 34)));
        m_resultsList->setItemWidget(item, buildMemberRow(m_userController->userById(key), memberDuties));
    }
}

void ScheduleTab::assignClicked()
{
    if (!m_isAdmin || !selectedSundayEditable()) {
        return;
    }
    AssignDutyDialog dialog(Duty(), m_selectedDate, m_userController->allUsers(), m_dutyTypeController, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    Duty newDuty = dialog.duty();
    if (!m_dutyController->addDuty(newDuty)) {
        QMessageBox::critical(this, QStringLiteral("Assign Duty"), m_dutyController->lastError());
        return;
    }
    populateSundayList();
    selectSunday(m_selectedDate);
}

void ScheduleTab::addMemberClicked()
{
    if (!m_isAdmin || !selectedSundayEditable()) {
        return;
    }
    AddToScheduleDialog dialog(m_selectedDate, m_userController->allUsers(), m_dutyTypeController, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    // A duty the member already has on this Sunday isn't added twice.
    const QVector<Duty> existing = m_dutyController->dutiesForDate(m_selectedDate);
    QStringList skipped;
    for (Duty duty : dialog.duties()) {
        const bool alreadyThere = std::any_of(existing.cbegin(), existing.cend(), [&duty](const Duty &other) {
            return other.memberId() == duty.memberId() && other.dutyTypeId() == duty.dutyTypeId();
        });
        if (alreadyThere) {
            skipped.append(m_dutyTypeController->dutyTypeById(duty.dutyTypeId()).name());
            continue;
        }
        if (!m_dutyController->addDuty(duty)) {
            QMessageBox::critical(this, QStringLiteral("Add Member"), m_dutyController->lastError());
            break;
        }
    }
    populateSundayList();
    selectSunday(m_selectedDate);
    if (!skipped.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Add Member"),
            QStringLiteral("Already on this Sunday, so not added again: %1.").arg(skipped.join(QStringLiteral(", "))));
    }
}

void ScheduleTab::assignForSelectedMemberClicked()
{
    if (!m_isAdmin || !selectedSundayEditable() || m_selectedDutyId < 0) {
        return;
    }
    const Duty reference = m_dutyController->dutyById(m_selectedDutyId);
    if (reference.id() < 0) {
        return;
    }
    // Prefill just the Member -- duty type/support/notes start blank, since this
    // is a brand-new duty for the same person, not an edit of the
    // one that's currently selected.
    Duty prefilled;
    prefilled.setMemberId(reference.memberId());
    AssignDutyDialog dialog(prefilled, m_selectedDate, m_userController->allUsers(), m_dutyTypeController, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    Duty newDuty = dialog.duty();
    if (!m_dutyController->addDuty(newDuty)) {
        QMessageBox::critical(this, QStringLiteral("Assign Another Duty"), m_dutyController->lastError());
        return;
    }
    populateSundayList();
    selectSunday(m_selectedDate);
}

void ScheduleTab::editClicked()
{
    if (!m_isAdmin || !selectedSundayEditable() || m_selectedDutyId < 0) {
        return;
    }
    if (m_selectedMemberId > 0) {
        editMemberOnSchedule(m_selectedMemberId);
        return;
    }
    editDuty(m_selectedDutyId);
}

void ScheduleTab::editDuty(int dutyId)
{
    if (!m_isAdmin || !selectedSundayEditable()) {
        return;
    }
    const Duty existing = m_dutyController->dutyById(dutyId);
    if (existing.id() < 0) {
        return;
    }
    AssignDutyDialog dialog(existing, existing.serviceDate(), m_userController->allUsers(), m_dutyTypeController, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    Duty updated = dialog.duty();
    if (!m_dutyController->updateDuty(updated)) {
        QMessageBox::critical(this, QStringLiteral("Edit Duty"), m_dutyController->lastError());
        return;
    }
    populateSundayList();
    selectSunday(m_selectedDate);
}

void ScheduleTab::deleteClicked()
{
    if (!m_isAdmin || !selectedSundayEditable() || m_selectedDutyId < 0) {
        return;
    }
    if (m_selectedMemberId > 0) {
        QVector<int> dutyIds;
        for (const Duty &duty : m_dutyController->dutiesForDate(m_selectedDate)) {
            if (duty.memberId() == m_selectedMemberId) {
                dutyIds.append(duty.id());
            }
        }
        const QString name = m_userController->userById(m_selectedMemberId).name();
        const QString question = QStringLiteral("Remove %1 from this Sunday? This deletes their %2 duties.")
            .arg(name).arg(dutyIds.size());
        if (QMessageBox::question(this, QStringLiteral("Remove Member"), question) != QMessageBox::Yes) {
            return;
        }
        for (int dutyId : std::as_const(dutyIds)) {
            if (!m_dutyController->removeDuty(dutyId)) {
                QMessageBox::critical(this, QStringLiteral("Remove Member"), m_dutyController->lastError());
                break;
            }
        }
        populateSundayList();
        selectSunday(m_selectedDate);
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("Delete Duty"), QStringLiteral("Delete this duty?"))
        != QMessageBox::Yes) {
        return;
    }
    if (!m_dutyController->removeDuty(m_selectedDutyId)) {
        QMessageBox::critical(this, QStringLiteral("Delete Duty"), m_dutyController->lastError());
        return;
    }
    populateSundayList();
    selectSunday(m_selectedDate);
}

void ScheduleTab::memberDoubleClicked(QListWidgetItem *item)
{
    if (!item) {
        return;
    }
    const int dutyId = item->data(Qt::UserRole).toInt();
    const Duty duty = m_dutyController->dutyById(dutyId);
    if (duty.id() < 0) {
        return;
    }
    // An Admin on an upcoming Sunday edits the member's place on it;
    // otherwise (logged out, or a past Sunday) it's the read-only summary.
    if (m_isAdmin && selectedSundayEditable()) {
        if (duty.memberId() > 0) {
            editMemberOnSchedule(duty.memberId());
        } else {
            editClicked(); // an unfilled duty: pick someone for it
        }
        return;
    }
    if (duty.memberId() <= 0) {
        return;
    }
    const User user = m_userController->userById(duty.memberId());
    if (user.id() < 0) {
        return;
    }
    MemberStatsDialog dialog(user, m_dutyController->allDutiesForMember(user.id()), m_dutyTypeController, this);
    dialog.exec();
}

void ScheduleTab::editMemberOnSchedule(int memberId)
{
    QVector<Duty> current;
    for (const Duty &duty : m_dutyController->dutiesForDate(m_selectedDate)) {
        if (duty.memberId() == memberId) {
            current.append(duty);
        }
    }
    if (current.isEmpty()) {
        return;
    }
    AddToScheduleDialog dialog(m_selectedDate, m_userController->allUsers(), m_dutyTypeController, this, current);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    // Keep duties still wanted (updating backup/notes, unless they differ
    // per duty and were left alone), remove the ones taken off, add the
    // new ones.
    const QVector<Duty> wanted = dialog.duties();
    const bool keepBackups = dialog.keepsEachBackup();
    const bool keepNotes = dialog.keepsEachNotes();
    bool ok = true;
    for (const Duty &existing : std::as_const(current)) {
        const auto match = std::find_if(wanted.cbegin(), wanted.cend(), [&existing](const Duty &duty) {
            return duty.dutyTypeId() == existing.dutyTypeId();
        });
        if (match == wanted.cend()) {
            ok = m_dutyController->removeDuty(existing.id());
            if (!ok) {
                break;
            }
            continue;
        }
        Duty updated = existing;
        if (!keepBackups) {
            updated.setSupportMemberId(match->supportMemberId());
        }
        if (!keepNotes) {
            updated.setNotes(match->notes());
        }
        if (updated.supportMemberId() != existing.supportMemberId() || updated.notes() != existing.notes()) {
            ok = m_dutyController->updateDuty(updated);
        }
        if (!ok) {
            break;
        }
    }
    for (Duty duty : wanted) {
        if (!ok) {
            break;
        }
        const bool alreadyThere = std::any_of(current.cbegin(), current.cend(), [&duty](const Duty &existing) {
            return existing.dutyTypeId() == duty.dutyTypeId();
        });
        if (!alreadyThere) {
            ok = m_dutyController->addDuty(duty);
        }
    }
    if (!ok) {
        QMessageBox::critical(this, QStringLiteral("Edit Member"), m_dutyController->lastError());
    }
    populateSundayList();
    selectSunday(m_selectedDate);
}

void ScheduleTab::refresh()
{
    const QDate previouslySelected = m_selectedDate;
    populateSundayList();
    selectSunday(previouslySelected);
    updateCopyPasteState();
}

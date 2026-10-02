#include "ScheduleTab.h"

#include "Style.h"

#include <algorithm>

#include <QLocale>
#include <QBrush>
#include <QCheckBox>
#include <QEvent>
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
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include "AssignDutyDialog.h"
#include "ActionBar.h"
#include "AddToScheduleDialog.h"
#include "ElidedLabel.h"
#include "MemberSundayDialog.h"
#include "Controllers/TeamController.h"
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
    // Upcoming Sundays run through the end of October next year (past
    // ones are listed only when they had a schedule).
    QDate sundayListEnd(const QDate &today)
    {
        return QDate(today.year() + 1, 10, 31);
    }

    QString formatSunday(const QDate &date)
    {
        // e.g. "Sun 6 Sep 2026", per SCHEDULING_FUNCTIONAL_REQUIREMENTS.md.
        return QLocale().toString(date, QStringLiteral("ddd d MMM yyyy"));
    }

    // Every way someone might type a Sunday into the search box: the list's
    // own "Sun 6 Sep 2026", the spelled-out "Sunday 6 September 2026",
    // "06/09/2026" and ISO "2026-09-06".
    QString sundaySearchText(const QDate &date)
    {
        return QStringList{
            formatSunday(date),
            QLocale().toString(date, QStringLiteral("dddd d MMMM yyyy")),
            date.toString(QStringLiteral("dd/MM/yyyy")),
            date.toString(Qt::ISODate),
        }.join(QLatin1Char(' ')).toLower();
    }
}

ScheduleTab::ScheduleTab(
    DutyController *dutyController,
    UserController *userController,
    DutyTypeController *dutyTypeController,
    TeamController *teamController,
    QWidget *parent)
    : QWidget(parent)
    , m_dutyController(dutyController)
    , m_userController(userController)
    , m_dutyTypeController(dutyTypeController)
    , m_teamController(teamController)
{
    auto *title = new QLabel(tr("Schedule"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(
        tr("Pick a Sunday to see who's serving, and assign duties for it. "
           "Double-click a member to see their part in that Sunday."),
        this);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));
    subtitle->setWordWrap(true);

    m_assignButton = new QPushButton(tr("Assign Duty"), this);
    connect(m_assignButton, &QPushButton::clicked, this, &ScheduleTab::assignClicked);
    m_copyButton = new QPushButton(tr("Copy Schedule"), this);
    m_copyButton->setObjectName(QStringLiteral("secondaryButton"));
    m_copyButton->setToolTip(tr("Copy this Sunday's duties (Ctrl+C)"));
    connect(m_copyButton, &QPushButton::clicked, this, &ScheduleTab::copyScheduleClicked);
    m_pasteButton = new QPushButton(tr("Paste Schedule"), this);
    m_pasteButton->setObjectName(QStringLiteral("secondaryButton"));
    m_pasteButton->setToolTip(tr("Paste the copied duties onto this Sunday (Ctrl+V)"));
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
    // Rows always fit the list's width (names shorten instead), so there's
    // never anything to scroll sideways to.
    m_resultsList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_resultsList->viewport()->installEventFilter(this);
    connect(m_resultsList, &QListWidget::currentRowChanged, this, [this](int row) {
        QListWidgetItem *item = row >= 0 ? m_resultsList->item(row) : nullptr;
        m_selectedDutyId = item ? item->data(Qt::UserRole).toInt() : -1;
        m_selectedMemberId = item && item->data(Qt::UserRole + 1).isValid()
            ? item->data(Qt::UserRole + 1).toInt() : -1;
        updateActionState();
    });
    connect(m_resultsList, &QListWidget::itemDoubleClicked, this, &ScheduleTab::memberDoubleClicked);

    m_combineCheck = new QCheckBox(tr("Combine each member's duties into one row"), this);
    m_combineCheck->setToolTip(tr("Show one row per member with all their duties. Each duty keeps its own backup "
        "and can still be edited on its own."));
    connect(m_combineCheck, &QCheckBox::toggled, this, &ScheduleTab::rebuildResults);

    m_editButton = new QPushButton(tr("Edit"), this);
    m_editButton->setObjectName(QStringLiteral("secondaryButton"));
    connect(m_editButton, &QPushButton::clicked, this, &ScheduleTab::editClicked);
    m_deleteButton = new QPushButton(tr("Delete"), this);
    m_deleteButton->setObjectName(QStringLiteral("dangerButton"));
    connect(m_deleteButton, &QPushButton::clicked, this, &ScheduleTab::deleteClicked);

    auto *actionBar = new ActionBar(this);
    actionBar->addWidget(m_assignButton);
    actionBar->addWidget(m_editButton);
    actionBar->addSeparator();
    actionBar->addWidget(m_copyButton);
    actionBar->addWidget(m_pasteButton);
    actionBar->addWidget(m_copiedLabel);
    actionBar->addStretch();
    actionBar->addWidget(m_deleteButton);

    m_pastNotice = new QLabel(tr("This Sunday has passed, so its schedule is read-only."), this);
    m_pastNotice->setObjectName(QStringLiteral("accentLabel"));
    m_pastNotice->setWordWrap(true);
    m_pastNotice->hide();
    setAdminMode(false);

    auto *resultsLayout = new QVBoxLayout;
    resultsLayout->addWidget(m_pastNotice);
    resultsLayout->addWidget(m_combineCheck);
    resultsLayout->addWidget(m_resultsList);
    auto *resultsBox = new QGroupBox(tr("Duties"), this);
    resultsBox->setLayout(resultsLayout);

    m_sundaySearch = new QLineEdit(this);
    m_sundaySearch->setFixedWidth(200);
    m_sundaySearch->setPlaceholderText(tr("Search date, duty, name..."));
    m_sundaySearch->setToolTip(tr("Find Sundays by date (e.g. \"27 Sep\" or \"October\"), by duty, or by a member's first or last name"));
    m_sundaySearch->setClearButtonEnabled(true);
    connect(m_sundaySearch, &QLineEdit::textChanged, this, &ScheduleTab::sundaySearchChanged);

    m_noSundayMatchLabel = new QLabel(tr("No Sundays match."), this);
    m_noSundayMatchLabel->setObjectName(QStringLiteral("mutedLabel"));
    m_noSundayMatchLabel->hide();

    auto *sundayBox = new QGroupBox(tr("Sundays"), this);
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
    // Name columns never get narrower than this, nor wider than the max;
    // between the two they follow the longest name on the Sunday.
    constexpr int kMinNameWidth = 70;
    constexpr int kMaxNameWidth = 240;
    constexpr int kNameToDutyGap = 16;

    QLabel *makeDutyPill(const QString &text, QWidget *parent)
    {
        auto *pill = new QLabel(text, parent);
        pill->setObjectName(QStringLiteral("dutyPill"));
        pill->setToolTip(text);
        pill->ensurePolished();
        // Always its natural size: squeezed shorter (e.g. in a combined
        // member row) the rounded background would lose its corners.
        // A few spare pixels so emoji (drawn from a fallback font that can
        // run wider than measured) never eat into the padding.
        pill->setFixedSize(pill->sizeHint() + QSize(6, 2));
        return pill;
    }

    // Fits the list row to its widget, so nothing (e.g. the backup's
    // badge) is cut off at the bottom.
    void setRowWidget(QListWidget *list, QListWidgetItem *item, QWidget *row)
    {
        row->ensurePolished();
        if (row->layout()) {
            row->layout()->activate();
        }
        const int height = std::max(row->sizeHint().height(), row->minimumSizeHint().height());
        item->setSizeHint(QSize(0, std::max(52, height + 4)));
        list->setItemWidget(item, row);
    }
}

QWidget *ScheduleTab::buildMemberColumn(const QString &name, const QString &nameStyle, const QString &notes,
                                       QWidget *parent)
{
    auto *column = new QWidget(parent);
    column->setFixedWidth(kMinNameWidth); // widened by fitMemberColumns()
    auto *layout = new QVBoxLayout(column);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(1);
    layout->addStretch();
    auto *nameLabel = new ElidedLabel(name, column);
    nameLabel->setObjectName(nameStyle);
    nameLabel->ensurePolished();
    layout->addWidget(nameLabel);
    int wanted = nameLabel->sizeHint().width();
    if (!notes.isEmpty()) {
        auto *notesLabel = new ElidedLabel(notes, column);
        notesLabel->setObjectName(QStringLiteral("mutedLabel"));
        notesLabel->ensurePolished();
        layout->addWidget(notesLabel);
        wanted = std::max(wanted, notesLabel->sizeHint().width());
    }
    layout->addStretch();
    m_nameColumnWidth = std::clamp(std::max(m_nameColumnWidth, wanted), kMinNameWidth, kMaxNameWidth);
    m_memberColumns.append(column);
    return column;
}

void ScheduleTab::fitMemberColumns()
{
    if (m_fitting) {
        return;
    }
    m_fitting = true;
    const int available = m_resultsList->viewport()->width();
    // Side by side while the shortest name column still fits beside it.
    const bool stacked = available < m_rowFixedWidth + kMinNameWidth;
    if (stacked != m_rowsStacked) {
        setRowsStacked(stacked);
    }
    const int fixed = stacked ? m_rowFixedWidthStacked : m_rowFixedWidth;
    const int width = std::clamp(available - fixed, kMinNameWidth, std::max(kMinNameWidth, m_nameColumnWidth));
    for (const QPointer<QWidget> &column : std::as_const(m_memberColumns)) {
        if (column && column->width() != width) {
            column->setFixedWidth(width);
        }
    }
    m_fitting = false;
}

void ScheduleTab::setRowsStacked(bool stacked)
{
    m_rowsStacked = stacked;
    for (const QPointer<QWidget> &content : std::as_const(m_dutyContents)) {
        if (auto *layout = content ? qobject_cast<QBoxLayout *>(content->layout()) : nullptr) {
            layout->setDirection(stacked ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
            layout->setSpacing(stacked ? 4 : 12);
        }
    }
    updateRowHeights();
}

void ScheduleTab::updateRowHeights()
{
    for (int i = 0; i < m_resultsList->count(); ++i) {
        QListWidgetItem *item = m_resultsList->item(i);
        QWidget *row = m_resultsList->itemWidget(item);
        if (row && row->layout()) {
            row->layout()->activate();
            const int height = std::max(row->sizeHint().height(), row->minimumSizeHint().height());
            item->setSizeHint(QSize(0, std::max(52, height + 4)));
        }
    }
}

bool ScheduleTab::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_resultsList->viewport() && event->type() == QEvent::Resize) {
        fitMemberColumns();
    }
    return QWidget::eventFilter(watched, event);
}

// "Backup", the backup's small badge and their name on one line (or a
// dash), so the badge sits level with the caption instead of under it.
// The name shortens before the badge or caption would be hidden.
QWidget *ScheduleTab::buildBackupLine(const Duty &duty, QWidget *parent)
{
    auto *line = new QWidget(parent);
    auto *layout = new QHBoxLayout(line);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    auto *caption = new QLabel(tr("Backup"), line);
    caption->setObjectName(QStringLiteral("dutyCaption"));
    layout->addWidget(caption, 0, Qt::AlignVCenter);
    const User backup = duty.supportMemberId() > 0 ? m_userController->userById(duty.supportMemberId()) : User();
    if (backup.id() >= 0) {
        layout->addWidget(MemberBadge::make(backup.name(), backup.color(), 20, line), 0, Qt::AlignVCenter);
        auto *backupName = new ElidedLabel(backup.name(), line);
        backupName->setObjectName(QStringLiteral("dutyBackupName"));
        layout->addWidget(backupName, 1, Qt::AlignVCenter);
    } else {
        auto *none = new QLabel(QStringLiteral("—"), line);
        none->setObjectName(QStringLiteral("mutedLabel"));
        layout->addWidget(none, 1, Qt::AlignVCenter);
    }
    return line;
}

// The duty's pill in a column m_dutyColumnWidth wide, so the backups
// after it line up from row to row.
QWidget *ScheduleTab::buildDutyCell(const Duty &duty, QWidget *parent)
{
    auto *cell = new QWidget(parent);
    cell->setFixedWidth(m_dutyColumnWidth);
    auto *layout = new QHBoxLayout(cell);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(makeDutyPill(m_dutyTypeController->dutyTypeById(duty.dutyTypeId()).iconAndName(), cell), 0, Qt::AlignVCenter);
    layout->addStretch();
    return cell;
}

// Opens Edit Duty for just this duty. Only added for an Admin on an
// upcoming Sunday.
QToolButton *ScheduleTab::buildEditButton(int dutyId, QWidget *parent)
{
    auto *editButton = new QToolButton(parent);
    editButton->setObjectName(QStringLiteral("dutyEditButton"));
    editButton->setText(tr("Edit"));
    editButton->setToolTip(tr("Edit just this duty and its backup"));
    editButton->setCursor(Qt::PointingHandCursor);
    connect(editButton, &QToolButton::clicked, this, [this, dutyId] { editDuty(dutyId); });
    return editButton;
}

// One duty, laid out across the full row: the member (badge, name in
// bold, any notes beneath), the duty as an icon pill, the backup, and
// (for an Admin on an upcoming Sunday) an Edit button.
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
    layout->addWidget(member.id() >= 0
        ? buildMemberColumn(member.name(), QStringLiteral("dutyMemberName"), duty.notes(), row)
        : buildMemberColumn(tr("Nobody assigned"), QStringLiteral("dutyUnfilled"), duty.notes(), row));
    layout->addSpacing(kNameToDutyGap);
    layout->addWidget(buildDutyContent(duty, row), 1);
    if (m_isAdmin && selectedSundayEditable()) {
        layout->addWidget(buildEditButton(duty.id(), row));
    }
    return row;
}

QWidget *ScheduleTab::buildDutyContent(const Duty &duty, QWidget *parent)
{
    auto *content = new QWidget(parent);
    auto *layout = new QBoxLayout(m_rowsStacked ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight, content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(m_rowsStacked ? 4 : 12);
    layout->addWidget(buildDutyCell(duty, content));
    layout->addWidget(buildBackupLine(duty, content), 1);
    m_dutyContents.append(content);
    return content;
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

    layout->addWidget(MemberBadge::make(member.name(), member.color(), 32, row), 0, Qt::AlignTop);
    QStringList notes;
    for (const Duty &duty : duties) {
        if (!duty.notes().isEmpty() && !notes.contains(duty.notes())) {
            notes.append(duty.notes());
        }
    }
    auto *memberColumn = buildMemberColumn(member.name(), QStringLiteral("dutyMemberName"),
                                           notes.join(QStringLiteral(" · ")), row);
    layout->addWidget(memberColumn, 0, Qt::AlignTop);
    layout->addSpacing(kNameToDutyGap);

    const bool canEdit = m_isAdmin && selectedSundayEditable();
    auto *dutiesColumn = new QWidget(row);
    auto *dutiesLayout = new QVBoxLayout(dutiesColumn);
    dutiesLayout->setContentsMargins(0, 0, 0, 0);
    dutiesLayout->setSpacing(6);
    for (const Duty &duty : duties) {
        auto *line = new QWidget(dutiesColumn);
        auto *lineLayout = new QHBoxLayout(line);
        lineLayout->setContentsMargins(0, 1, 0, 1);
        lineLayout->setSpacing(12);
        lineLayout->addWidget(buildDutyContent(duty, line), 1);
        if (canEdit) {
            lineLayout->addWidget(buildEditButton(duty.id(), line));
        }
        dutiesLayout->addWidget(line);
    }
    layout->addWidget(dutiesColumn, 1);

    return row;
}

void ScheduleTab::setAdminMode(bool isAdmin)
{
    m_isAdmin = isAdmin;
    m_assignButton->setVisible(isAdmin);
    m_editButton->setVisible(isAdmin);
    m_deleteButton->setVisible(isAdmin);
    m_copyButton->setVisible(isAdmin);
    m_pasteButton->setVisible(isAdmin);
    m_copiedLabel->setVisible(isAdmin);
    updateActionState();
    // Rows carry per-duty Edit buttons only an Admin sees.
    if (m_selectedDate.isValid()) {
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
    m_editButton->setEnabled(editable && hasDuty);
    m_deleteButton->setEnabled(editable && hasDuty);
    // On a combined row they act on the member's whole place this Sunday.
    const bool memberRow = m_selectedMemberId > 0;
    m_editButton->setText(memberRow ? tr("Edit Member") : tr("Edit"));
    m_deleteButton->setText(memberRow ? tr("Remove Member") : tr("Delete"));
    m_pastNotice->setVisible(m_selectedDate.isValid() && !selectedSundayEditable());
    updateCopyPasteState();
}

void ScheduleTab::updateCopyPasteState()
{
    m_copyButton->setEnabled(m_isAdmin && m_datesWithDuties.contains(m_selectedDate));
    m_pasteButton->setEnabled(m_isAdmin && selectedSundayEditable()
                              && m_copiedDate.isValid() && m_copiedDate != m_selectedDate);
    m_copiedLabel->setText(m_copiedDate.isValid()
        ? tr("Copied: %1").arg(formatSunday(m_copiedDate))
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
        QMessageBox::information(this, tr("Paste Schedule"),
            tr("%1 no longer has any duties to copy.").arg(formatSunday(fromDate)));
        m_copiedDate = QDate();
        updateCopyPasteState();
        return;
    }
    const int existingCount = m_dutyController->dutiesForDate(toDate).size();

    bool replaceExisting = false;
    if (existingCount == 0) {
        const QString question = tr("Copy %1 duties from %2 onto %3?")
            .arg(sourceCount).arg(formatSunday(fromDate), formatSunday(toDate));
        if (QMessageBox::question(this, tr("Paste Schedule"), question) != QMessageBox::Yes) {
            return;
        }
    } else {
        QMessageBox box(QMessageBox::Question, tr("Paste Schedule"),
            tr("%1 already has %2 duties.").arg(formatSunday(toDate)).arg(existingCount),
            QMessageBox::NoButton, this);
        box.setInformativeText(tr("Add to them: keeps what's there and adds the copied duties (skipping any duty the same "
            "member already has).\n\nReplace them: deletes this Sunday's duties, including any "
            "time-off requests members sent for them, then pastes the copied schedule."));
        QPushButton *addButton = box.addButton(tr("Add to Them"), QMessageBox::AcceptRole);
        QPushButton *replaceButton = box.addButton(tr("Replace Them"), QMessageBox::DestructiveRole);
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
        QMessageBox::critical(this, tr("Paste Schedule"), m_dutyController->lastError());
        return;
    }
    populateSundayList();
    selectSunday(toDate);

    QString summary = tr("Pasted %1 duties onto %2.").arg(copied).arg(formatSunday(toDate));
    if (skipped > 0) {
        summary += tr("\nSkipped %1 already on that Sunday.").arg(skipped);
    }
    const QStringList unavailable = m_dutyController->membersMarkedUnavailable(toDate);
    if (!unavailable.isEmpty()) {
        summary += tr("\n\nHeads up: these members marked themselves unavailable that day:\n  %1")
            .arg(unavailable.join(QStringLiteral("\n  ")));
    }
    QMessageBox::information(this, tr("Paste Schedule"), summary);
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
    const QDate upcoming = nearestSunday(today);
    const QDate end = sundayListEnd(today);
    // Past Sundays that had a schedule come first, oldest at the top, so
    // what was served can still be looked at (and copied) from here.
    QVector<QDate> pastSundays;
    for (const QDate &date : std::as_const(m_datesWithDuties)) {
        if (date < upcoming) {
            pastSundays.append(date);
        }
    }
    std::sort(pastSundays.begin(), pastSundays.end());
    for (const QDate &sunday : std::as_const(pastSundays)) {
        auto *item = new QListWidgetItem(formatSunday(sunday), m_sundayList);
        item->setData(Qt::UserRole, sunday);
        applySundayItemStyle(item, false);
    }
    for (QDate sunday = upcoming; sunday <= end; sunday = sunday.addDays(7)) {
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
    } else if (date < nearestSunday(QDate::currentDate())) {
        // Past: orange.
        item->setBackground(isDark ? QColor(0x3a, 0x22, 0x10) : QColor(0xfd, 0xec, 0xdc));
        item->setForeground(isDark ? QColor(0xf0, 0xa3, 0x5e) : QColor(0xb3, 0x54, 0x1e));
    } else if (hasDuties && date == nearestSunday(QDate::currentDate())) {
        // The upcoming Sunday, once it has duties: green.
        item->setBackground(isDark ? QColor(0x12, 0x30, 0x1c) : QColor(0xe3, 0xf4, 0xe8));
        item->setForeground(isDark ? QColor(0x6f, 0xcf, 0x8a) : QColor(0x1e, 0x7b, 0x3a));
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
    m_memberColumns.clear();
    m_dutyContents.clear();
    m_nameColumnWidth = 0;
    m_rowFixedWidth = 0;
    m_selectedDutyId = -1;
    m_selectedMemberId = -1;
    updateActionState();

    const QVector<Duty> duties = m_dutyController->dutiesForDate(m_selectedDate);
    if (duties.isEmpty()) {
        auto *item = new QListWidgetItem(m_resultsList);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        m_resultsList->addItem(item);
        m_resultsList->setItemWidget(item, new QLabel(tr("No duties for this date."), m_resultsList));
        return;
    }
    m_dutyColumnWidth = 0;
    for (const Duty &duty : duties) {
        QLabel *probe = makeDutyPill(m_dutyTypeController->dutyTypeById(duty.dutyTypeId()).iconAndName(), m_resultsList);
        m_dutyColumnWidth = std::max(m_dutyColumnWidth, probe->minimumWidth());
        delete probe;
    }

    if (!m_combineCheck->isChecked()) {
        for (const Duty &duty : duties) {
            auto *item = new QListWidgetItem(m_resultsList);
            item->setData(Qt::UserRole, duty.id());
            m_resultsList->addItem(item);
            setRowWidget(m_resultsList, item, buildRow(duty));
        }
        finishRows();
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
            setRowWidget(m_resultsList, item, buildRow(memberDuties.first()));
            continue;
        }
        item->setData(Qt::UserRole + 1, key);
        setRowWidget(m_resultsList, item, buildMemberRow(m_userController->userById(key), memberDuties));
    }
    finishRows();
}

void ScheduleTab::finishRows()
{
    // How wide each row is besides its name column (taken while the name
    // columns are at their minimum), side by side and stacked, so
    // fitMemberColumns knows what's left in each arrangement.
    const bool wasStacked = m_rowsStacked;
    auto measure = [this](bool stacked) {
        setRowsStacked(stacked);
        int fixed = 0;
        for (int i = 0; i < m_resultsList->count(); ++i) {
            QWidget *row = m_resultsList->itemWidget(m_resultsList->item(i));
            if (row && row->layout()) {
                row->layout()->activate();
                fixed = std::max(fixed, row->minimumSizeHint().width() - kMinNameWidth);
            }
        }
        return fixed;
    };
    m_rowFixedWidth = measure(false);
    m_rowFixedWidthStacked = std::min(m_rowFixedWidth, measure(true));
    setRowsStacked(wasStacked);
    // The list never gets narrower than a stacked row with the shortest
    // name column, so the window stops shrinking before anything is cut off.
    const int scrollBar = m_resultsList->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    m_resultsList->setMinimumWidth(m_rowFixedWidthStacked + kMinNameWidth + 2 * m_resultsList->frameWidth() + scrollBar + 2);
    fitMemberColumns();
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
        QMessageBox::critical(this, tr("Assign Duty"), m_dutyController->lastError());
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
        QMessageBox::critical(this, tr("Edit Duty"), m_dutyController->lastError());
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
        const QString question = tr("Remove %1 from this Sunday? This deletes their %2 duties.")
            .arg(name).arg(dutyIds.size());
        if (QMessageBox::question(this, tr("Remove Member"), question) != QMessageBox::Yes) {
            return;
        }
        for (int dutyId : std::as_const(dutyIds)) {
            if (!m_dutyController->removeDuty(dutyId)) {
                QMessageBox::critical(this, tr("Remove Member"), m_dutyController->lastError());
                break;
            }
        }
        populateSundayList();
        selectSunday(m_selectedDate);
        return;
    }
    if (QMessageBox::question(this, tr("Delete Duty"), tr("Delete this duty?"))
        != QMessageBox::Yes) {
        return;
    }
    if (!m_dutyController->removeDuty(m_selectedDutyId)) {
        QMessageBox::critical(this, tr("Delete Duty"), m_dutyController->lastError());
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
    const bool canEdit = m_isAdmin && selectedSundayEditable();
    if (duty.memberId() <= 0) {
        if (canEdit) {
            editClicked(); // an unfilled duty: pick someone for it
        }
        return;
    }
    const User user = m_userController->userById(duty.memberId());
    if (user.id() < 0) {
        return;
    }

    // Their part in the selected Sunday, with the way into editing it (for
    // an Admin on an upcoming Sunday) or into their full history.
    const QString teamName = user.teamId() > 0 ? m_teamController->teamById(user.teamId()).name() : QString();
    const bool markedAway = m_dutyController->membersMarkedUnavailable(m_selectedDate).contains(user.name());
    MemberSundayDialog dialog(user, teamName, m_selectedDate, m_dutyController->dutiesForDate(m_selectedDate),
                              markedAway, canEdit, m_userController, m_dutyTypeController, this);
    dialog.exec();
    if (dialog.editRequested()) {
        editMemberOnSchedule(user.id());
    } else if (dialog.allDutiesRequested()) {
        MemberStatsDialog stats(user, m_dutyController->allDutiesForMember(user.id()), m_dutyTypeController, this);
        stats.exec();
    }
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
        QMessageBox::critical(this, tr("Edit Member"), m_dutyController->lastError());
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

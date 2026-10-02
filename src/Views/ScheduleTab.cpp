#include "ScheduleTab.h"

#include "Style.h"

#include <algorithm>

#include <QBrush>
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
#include <QVBoxLayout>

#include "AssignDutyDialog.h"
#include "MemberEditDialog.h"
#include "AvatarLoader.h"
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
    QNetworkAccessManager *networkManager,
    QWidget *parent)
    : QWidget(parent)
    , m_dutyController(dutyController)
    , m_userController(userController)
    , m_dutyTypeController(dutyTypeController)
    , m_networkManager(networkManager)
{
    auto *title = new QLabel(QStringLiteral("Schedule"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(
        QStringLiteral("Pick a Sunday to see who's serving, and assign duties for it. "
                        "Double-click a Member to see their responsibilities."),
        this);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));

    m_assignButton = new QPushButton(QStringLiteral("+  Assign Duty"), this);
    connect(m_assignButton, &QPushButton::clicked, this, &ScheduleTab::assignClicked);
    m_addMemberButton = new QPushButton(QStringLiteral("+  Add Member"), this);
    m_addMemberButton->setObjectName(QStringLiteral("secondaryButton"));
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

    auto *topRow = new QHBoxLayout;
    topRow->addWidget(m_assignButton);
    topRow->addWidget(m_addMemberButton);
    topRow->addSpacing(12);
    topRow->addWidget(m_copyButton);
    topRow->addWidget(m_pasteButton);
    topRow->addWidget(m_copiedLabel);
    topRow->addStretch();
    topRow->addWidget(m_assignForMemberButton);

    m_sundayList = new QListWidget(this);
    m_sundayList->setFixedWidth(200);
    connect(m_sundayList, &QListWidget::currentItemChanged, this, &ScheduleTab::sundaySelectionChanged);

    m_resultsList = new QListWidget(this);
    m_resultsList->setAlternatingRowColors(true);
    connect(m_resultsList, &QListWidget::currentRowChanged, this, [this](int row) {
        QListWidgetItem *item = row >= 0 ? m_resultsList->item(row) : nullptr;
        m_selectedDutyId = item ? item->data(Qt::UserRole).toInt() : -1;
        m_editButton->setEnabled(m_isAdmin && m_selectedDutyId >= 0);
        m_deleteButton->setEnabled(m_isAdmin && m_selectedDutyId >= 0);
        m_assignForMemberButton->setEnabled(m_isAdmin && m_selectedDutyId >= 0);
    });
    connect(m_resultsList, &QListWidget::itemDoubleClicked, this, &ScheduleTab::memberDoubleClicked);

    m_editButton = new QPushButton(QStringLiteral("Edit"), this);
    m_editButton->setObjectName(QStringLiteral("secondaryButton"));
    connect(m_editButton, &QPushButton::clicked, this, &ScheduleTab::editClicked);
    m_deleteButton = new QPushButton(QStringLiteral("Delete"), this);
    m_deleteButton->setObjectName(QStringLiteral("dangerButton"));
    connect(m_deleteButton, &QPushButton::clicked, this, &ScheduleTab::deleteClicked);
    setAdminMode(false);

    auto *bottomButtons = new QHBoxLayout;
    bottomButtons->addWidget(m_editButton);
    bottomButtons->addWidget(m_deleteButton);
    bottomButtons->addStretch();

    auto *resultsLayout = new QVBoxLayout;
    resultsLayout->addWidget(m_resultsList);
    resultsLayout->addLayout(bottomButtons);
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

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(6);
    layout->addLayout(topRow);
    layout->addSpacing(8);
    layout->addLayout(columns);

    populateSundayList();
    selectSunday(nearestSunday(QDate::currentDate()));
}

QString ScheduleTab::memberName(int userId) const
{
    if (userId <= 0) {
        return QStringLiteral("Unassigned");
    }
    const User user = m_userController->userById(userId);
    return user.id() >= 0 ? user.name() : QStringLiteral("(deleted member)");
}

QString ScheduleTab::memberAvatarSeed(int userId) const
{
    if (userId <= 0) {
        return QString();
    }
    const User user = m_userController->userById(userId);
    return user.id() >= 0 ? user.avatarSeed() : QString();
}

QWidget *ScheduleTab::buildRow(const Duty &duty)
{
    auto *row = new QWidget(m_resultsList);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(10);

    auto *avatar = new QLabel(row);
    const QString seed = memberAvatarSeed(duty.memberId());
    if (!seed.isEmpty() && m_networkManager) {
        AvatarLoader::loadInto(*m_networkManager, seed, avatar, 36);
    } else {
        avatar->setFixedSize(36, 36);
        avatar->setObjectName(QStringLiteral("avatarPlaceholder"));
    }
    layout->addWidget(avatar);

    const DutyType dutyType = m_dutyTypeController->dutyTypeById(duty.dutyTypeId());
    auto *dutyTypeIcon = new QLabel(dutyType.icon(), row);
    QFont iconFont = dutyTypeIcon->font();
    iconFont.setPointSize(16);
    dutyTypeIcon->setFont(iconFont);
    layout->addWidget(dutyTypeIcon);

    auto *textContainer = new QWidget(row);
    auto *textLayout = new QVBoxLayout(textContainer);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);
    auto *dutyTypeLabel = new QLabel(dutyType.name(), textContainer);
    dutyTypeLabel->setStyleSheet(QStringLiteral("font-weight: 600;"));

    // Main member and support member on the same line, e.g.
    // "Grace Adeyemi   ·   Support: Ruth Mensah".
    QString memberLine = memberName(duty.memberId());
    if (duty.supportMemberId() > 0) {
        memberLine += QStringLiteral("   ·   Support: %1").arg(memberName(duty.supportMemberId()));
    }
    auto *memberLabel = new QLabel(memberLine, textContainer);
    memberLabel->setObjectName(QStringLiteral("mutedLabel"));

    textLayout->addWidget(dutyTypeLabel);
    textLayout->addWidget(memberLabel);
    layout->addWidget(textContainer, 1);

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
    m_editButton->setEnabled(isAdmin && m_selectedDutyId >= 0);
    m_deleteButton->setEnabled(isAdmin && m_selectedDutyId >= 0);
    m_assignForMemberButton->setEnabled(isAdmin && m_selectedDutyId >= 0);
    m_copyButton->setVisible(isAdmin);
    m_pasteButton->setVisible(isAdmin);
    m_copiedLabel->setVisible(isAdmin);
    updateCopyPasteState();
}

void ScheduleTab::updateCopyPasteState()
{
    m_copyButton->setEnabled(m_isAdmin && m_datesWithDuties.contains(m_selectedDate));
    m_pasteButton->setEnabled(m_isAdmin && m_copiedDate.isValid() && m_copiedDate != m_selectedDate);
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
    if (!m_isAdmin || !m_copiedDate.isValid() || m_copiedDate == m_selectedDate) {
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

    const bool isBlack = currentTheme() == Theme::Black;
    if (isSelected) {
        item->setBackground(isBlack ? QColor(0x1f, 0x4a, 0x85) : QColor(0x14, 0x33, 0x5c));
        item->setForeground(QColor(Qt::white));
    } else if (hasDuties) {
        item->setBackground(isBlack ? QColor(0x2a, 0x22, 0x10) : QColor(0xfa, 0xf3, 0xe0));
        item->setForeground(isBlack ? QColor(0xe0, 0xb8, 0x5a) : QColor(0x8a, 0x6a, 0x1a));
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
    updateCopyPasteState();
}

void ScheduleTab::rebuildResults()
{
    m_resultsList->clear();
    m_selectedDutyId = -1;
    m_editButton->setEnabled(false);
    m_deleteButton->setEnabled(false);

    const QVector<Duty> duties = m_dutyController->dutiesForDate(m_selectedDate);
    if (duties.isEmpty()) {
        auto *item = new QListWidgetItem(m_resultsList);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        m_resultsList->addItem(item);
        m_resultsList->setItemWidget(item, new QLabel(QStringLiteral("No duties for this date."), m_resultsList));
        return;
    }
    for (const Duty &duty : duties) {
        auto *item = new QListWidgetItem(m_resultsList);
        item->setData(Qt::UserRole, duty.id());
        item->setSizeHint(QSize(0, 56));
        m_resultsList->addItem(item);
        m_resultsList->setItemWidget(item, buildRow(duty));
    }
}

void ScheduleTab::assignClicked()
{
    if (!m_isAdmin) {
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
    if (!m_isAdmin) {
        return;
    }
    MemberEditDialog dialog(User(), m_userController, m_networkManager, this);
    dialog.exec();
}

void ScheduleTab::assignForSelectedMemberClicked()
{
    if (!m_isAdmin || m_selectedDutyId < 0) {
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
    if (!m_isAdmin || m_selectedDutyId < 0) {
        return;
    }
    const Duty existing = m_dutyController->dutyById(m_selectedDutyId);
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
    if (!m_isAdmin || m_selectedDutyId < 0) {
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
    if (duty.id() < 0 || duty.memberId() <= 0) {
        return;
    }
    const User user = m_userController->userById(duty.memberId());
    if (user.id() < 0) {
        return;
    }
    MemberStatsDialog dialog(user, m_dutyController->allDutiesForMember(user.id()), m_dutyTypeController, m_networkManager, this);
    dialog.exec();
}

void ScheduleTab::refresh()
{
    const QDate previouslySelected = m_selectedDate;
    populateSundayList();
    selectSunday(previouslySelected);
    updateCopyPasteState();
}

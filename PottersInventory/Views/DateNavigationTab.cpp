#include "DateNavigationTab.h"

#include <QCalendarWidget>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QTextCharFormat>
#include <QVBoxLayout>

#include "AssignRoleDialog.h"
#include "AvatarLoader.h"
#include "Controllers/AssignmentController.h"
#include "Controllers/UserController.h"
#include "MemberStatsDialog.h"
#include "Models/Assignment.h"
#include "Models/User.h"
#include "RoleDisplay.h"

DateNavigationTab::DateNavigationTab(
    AssignmentController *assignmentController,
    UserController *userController,
    QNetworkAccessManager *networkManager,
    QWidget *parent)
    : QWidget(parent)
    , m_assignmentController(assignmentController)
    , m_userController(userController)
    , m_networkManager(networkManager)
{
    auto *title = new QLabel(QStringLiteral("Date"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(
        QStringLiteral("Pick a Sunday to see who's serving, and assign roles for it. "
                        "Double-click a Member to see their responsibilities."),
        this);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));

    m_assignButton = new QPushButton(QStringLiteral("+  Assign Role"), this);
    connect(m_assignButton, &QPushButton::clicked, this, &DateNavigationTab::assignClicked);
    auto *topRow = new QHBoxLayout;
    topRow->addWidget(m_assignButton);
    topRow->addStretch();

    m_calendar = new QCalendarWidget(this);
    m_calendar->setGridVisible(false);
    m_calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    connect(m_calendar, &QCalendarWidget::clicked, this, &DateNavigationTab::dateSelected);
    connect(m_calendar, &QCalendarWidget::selectionChanged, this,
            [this]() { dateSelected(m_calendar->selectedDate()); });

    m_resultsList = new QListWidget(this);
    m_resultsList->setAlternatingRowColors(true);
    connect(m_resultsList, &QListWidget::currentRowChanged, this, [this](int row) {
        QListWidgetItem *item = row >= 0 ? m_resultsList->item(row) : nullptr;
        m_selectedAssignmentId = item ? item->data(Qt::UserRole).toInt() : -1;
        m_editButton->setEnabled(m_isAdmin && m_selectedAssignmentId >= 0);
        m_deleteButton->setEnabled(m_isAdmin && m_selectedAssignmentId >= 0);
    });
    connect(m_resultsList, &QListWidget::itemDoubleClicked, this, &DateNavigationTab::memberDoubleClicked);

    m_editButton = new QPushButton(QStringLiteral("Edit"), this);
    m_editButton->setObjectName(QStringLiteral("secondaryButton"));
    connect(m_editButton, &QPushButton::clicked, this, &DateNavigationTab::editClicked);
    m_deleteButton = new QPushButton(QStringLiteral("Delete"), this);
    m_deleteButton->setObjectName(QStringLiteral("dangerButton"));
    connect(m_deleteButton, &QPushButton::clicked, this, &DateNavigationTab::deleteClicked);
    setAdminMode(false);

    auto *bottomButtons = new QHBoxLayout;
    bottomButtons->addWidget(m_editButton);
    bottomButtons->addWidget(m_deleteButton);
    bottomButtons->addStretch();

    auto *resultsLayout = new QVBoxLayout;
    resultsLayout->addWidget(m_resultsList);
    resultsLayout->addLayout(bottomButtons);
    auto *resultsBox = new QGroupBox(QStringLiteral("Assignments"), this);
    resultsBox->setLayout(resultsLayout);

    auto *columns = new QHBoxLayout;
    columns->setSpacing(16);
    columns->addWidget(m_calendar);
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

    updateCalendarHighlights();
    rebuildResults();
}

QString DateNavigationTab::memberName(int userId) const
{
    if (userId <= 0) {
        return QStringLiteral("Unassigned");
    }
    const User user = m_userController->userById(userId);
    return user.id() >= 0 ? user.name() : QStringLiteral("(deleted member)");
}

QString DateNavigationTab::memberAvatarSeed(int userId) const
{
    if (userId <= 0) {
        return QString();
    }
    const User user = m_userController->userById(userId);
    return user.id() >= 0 ? user.avatarSeed() : QString();
}

QWidget *DateNavigationTab::buildRow(const Assignment &assignment)
{
    auto *row = new QWidget(m_resultsList);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(10);

    auto *avatar = new QLabel(row);
    const QString seed = memberAvatarSeed(assignment.memberId());
    if (!seed.isEmpty() && m_networkManager) {
        AvatarLoader::loadInto(*m_networkManager, seed, avatar, 36);
    } else {
        avatar->setFixedSize(36, 36);
        avatar->setStyleSheet(QStringLiteral("background: #e5e7eb; border-radius: 4px;"));
    }
    layout->addWidget(avatar);

    auto *roleIcon = new QLabel(RoleDisplay::icon(assignment.role()), row);
    QFont iconFont = roleIcon->font();
    iconFont.setPointSize(16);
    roleIcon->setFont(iconFont);
    layout->addWidget(roleIcon);

    auto *textContainer = new QWidget(row);
    auto *textLayout = new QVBoxLayout(textContainer);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);
    auto *roleLabel = new QLabel(RoleDisplay::label(assignment.role()), textContainer);
    roleLabel->setStyleSheet(QStringLiteral("font-weight: 600;"));

    // Main member and support member on the same line, e.g.
    // "Grace Adeyemi   ·   Support: Ruth Mensah".
    QString memberLine = memberName(assignment.memberId());
    if (assignment.supportMemberId() > 0) {
        memberLine += QStringLiteral("   ·   Support: %1").arg(memberName(assignment.supportMemberId()));
    }
    auto *memberLabel = new QLabel(memberLine, textContainer);
    memberLabel->setStyleSheet(QStringLiteral("color: #666;"));

    textLayout->addWidget(roleLabel);
    textLayout->addWidget(memberLabel);
    layout->addWidget(textContainer, 1);

    return row;
}

void DateNavigationTab::setAdminMode(bool isAdmin)
{
    m_isAdmin = isAdmin;
    m_assignButton->setVisible(isAdmin);
    m_editButton->setVisible(isAdmin);
    m_deleteButton->setVisible(isAdmin);
    m_editButton->setEnabled(isAdmin && m_selectedAssignmentId >= 0);
    m_deleteButton->setEnabled(isAdmin && m_selectedAssignmentId >= 0);
}

void DateNavigationTab::updateCalendarHighlights()
{
    // Sunday is the day being scheduled -- call it out distinctly instead
    // of Qt's default plain-red "weekend" styling, and tone Saturday back
    // down to a normal weekday since it's not otherwise significant here.
    QTextCharFormat sundayFormat;
    sundayFormat.setForeground(QColor(0x14, 0x33, 0x5c));
    sundayFormat.setFontWeight(QFont::Bold);
    m_calendar->setWeekdayTextFormat(Qt::Sunday, sundayFormat);
    m_calendar->setWeekdayTextFormat(Qt::Saturday, QTextCharFormat());

    // Reset any previously-marked dates (an edited/deleted assignment
    // shouldn't leave a stale highlight behind), then mark every Sunday
    // that currently has at least one assignment.
    m_calendar->setDateTextFormat(QDate(), QTextCharFormat());
    QTextCharFormat scheduledFormat;
    scheduledFormat.setBackground(QColor(0xfa, 0xf3, 0xe0));
    scheduledFormat.setForeground(QColor(0x8a, 0x6a, 0x1a));
    scheduledFormat.setFontWeight(QFont::Bold);
    QSet<QDate> markedDates;
    for (const Assignment &assignment : m_assignmentController->allAssignments()) {
        if (markedDates.contains(assignment.serviceDate())) {
            continue;
        }
        markedDates.insert(assignment.serviceDate());
        m_calendar->setDateTextFormat(assignment.serviceDate(), scheduledFormat);
    }
}

void DateNavigationTab::rebuildResults()
{
    m_resultsList->clear();
    m_selectedAssignmentId = -1;
    m_editButton->setEnabled(false);
    m_deleteButton->setEnabled(false);

    const QDate date = m_calendar->selectedDate();
    const QVector<Assignment> assignments = m_assignmentController->assignmentsForDate(date);
    if (assignments.isEmpty()) {
        auto *item = new QListWidgetItem(m_resultsList);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        m_resultsList->addItem(item);
        m_resultsList->setItemWidget(item, new QLabel(QStringLiteral("No assignments for this date."), m_resultsList));
        return;
    }
    for (const Assignment &assignment : assignments) {
        auto *item = new QListWidgetItem(m_resultsList);
        item->setData(Qt::UserRole, assignment.id());
        item->setSizeHint(QSize(0, 56));
        m_resultsList->addItem(item);
        m_resultsList->setItemWidget(item, buildRow(assignment));
    }
}

void DateNavigationTab::dateSelected(const QDate &)
{
    rebuildResults();
}

void DateNavigationTab::assignClicked()
{
    if (!m_isAdmin) {
        return;
    }
    AssignRoleDialog dialog(Assignment(), m_calendar->selectedDate(), m_userController->allUsers(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    Assignment newAssignment = dialog.assignment();
    if (!m_assignmentController->addAssignment(newAssignment)) {
        QMessageBox::critical(this, QStringLiteral("Assign Role"), m_assignmentController->lastError());
        return;
    }
    updateCalendarHighlights();
    rebuildResults();
}

void DateNavigationTab::editClicked()
{
    if (!m_isAdmin || m_selectedAssignmentId < 0) {
        return;
    }
    const Assignment existing = m_assignmentController->assignmentById(m_selectedAssignmentId);
    if (existing.id() < 0) {
        return;
    }
    AssignRoleDialog dialog(existing, existing.serviceDate(), m_userController->allUsers(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    Assignment updated = dialog.assignment();
    if (!m_assignmentController->updateAssignment(updated)) {
        QMessageBox::critical(this, QStringLiteral("Edit Assignment"), m_assignmentController->lastError());
        return;
    }
    updateCalendarHighlights();
    rebuildResults();
}

void DateNavigationTab::deleteClicked()
{
    if (!m_isAdmin || m_selectedAssignmentId < 0) {
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("Delete Assignment"), QStringLiteral("Delete this assignment?"))
        != QMessageBox::Yes) {
        return;
    }
    if (!m_assignmentController->removeAssignment(m_selectedAssignmentId)) {
        QMessageBox::critical(this, QStringLiteral("Delete Assignment"), m_assignmentController->lastError());
        return;
    }
    updateCalendarHighlights();
    rebuildResults();
}

void DateNavigationTab::memberDoubleClicked(QListWidgetItem *item)
{
    if (!item) {
        return;
    }
    const int assignmentId = item->data(Qt::UserRole).toInt();
    const Assignment assignment = m_assignmentController->assignmentById(assignmentId);
    if (assignment.id() < 0 || assignment.memberId() <= 0) {
        return;
    }
    const User user = m_userController->userById(assignment.memberId());
    if (user.id() < 0) {
        return;
    }
    MemberStatsDialog dialog(user, m_assignmentController->allAssignmentsForMember(user.id()), m_networkManager, this);
    dialog.exec();
}

void DateNavigationTab::refresh()
{
    updateCalendarHighlights();
    rebuildResults();
}

#include "NonAvailabilityRequestsView.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "Controllers/AssignmentController.h"
#include "Controllers/NonAvailabilityRequestController.h"
#include "Controllers/UserController.h"
#include "Models/Assignment.h"
#include "Models/NonAvailabilityRequest.h"
#include "Models/User.h"
#include "RoleDisplay.h"

NonAvailabilityRequestsView::NonAvailabilityRequestsView(
    NonAvailabilityRequestController *requestController,
    AssignmentController *assignmentController,
    UserController *userController,
    QWidget *parent)
    : QWidget(parent)
    , m_requestController(requestController)
    , m_assignmentController(assignmentController)
    , m_userController(userController)
{
    auto *title = new QLabel(QStringLiteral("Non-Availability Requests"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(
        QStringLiteral("Pending requests from Members who can't make an assignment they've been scheduled for."), this);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));

    m_list = new QListWidget(this);
    m_list->setAlternatingRowColors(true);
    connect(m_list, &QListWidget::currentRowChanged, this, &NonAvailabilityRequestsView::selectionChanged);

    m_messageLabel = new QLabel(this);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setMinimumHeight(60);

    m_approveButton = new QPushButton(QStringLiteral("Approve"), this);
    m_denyButton = new QPushButton(QStringLiteral("Deny"), this);
    m_denyButton->setObjectName(QStringLiteral("dangerButton"));
    connect(m_approveButton, &QPushButton::clicked, this, &NonAvailabilityRequestsView::approveClicked);
    connect(m_denyButton, &QPushButton::clicked, this, &NonAvailabilityRequestsView::denyClicked);
    setAdminMode(false, -1);

    auto *buttons = new QHBoxLayout;
    buttons->addWidget(m_approveButton);
    buttons->addWidget(m_denyButton);

    auto *detailLayout = new QVBoxLayout;
    detailLayout->addWidget(m_messageLabel);
    detailLayout->addLayout(buttons);
    auto *detailBox = new QGroupBox(QStringLiteral("Request"), this);
    detailBox->setLayout(detailLayout);

    auto *listBox = new QGroupBox(QStringLiteral("Pending"), this);
    auto *listLayout = new QVBoxLayout;
    listLayout->addWidget(m_list);
    listBox->setLayout(listLayout);

    auto *columns = new QHBoxLayout;
    columns->setSpacing(16);
    columns->addWidget(listBox, 1);
    columns->addWidget(detailBox, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(6);
    layout->addLayout(columns);

    refresh();
}

void NonAvailabilityRequestsView::setAdminMode(bool isAdmin, int adminUserId)
{
    m_isAdmin = isAdmin;
    m_adminUserId = adminUserId;
    m_approveButton->setVisible(isAdmin);
    m_denyButton->setVisible(isAdmin);
}

void NonAvailabilityRequestsView::refresh()
{
    m_list->clear();
    m_messageLabel->clear();
    for (const NonAvailabilityRequest &request : m_requestController->listPending()) {
        const Assignment assignment = m_assignmentController->assignmentById(request.assignmentId());
        const User user = m_userController->userById(request.userId());
        const QString label = QStringLiteral("%1 — %2 — %3")
            .arg(assignment.serviceDate().toString(QStringLiteral("yyyy-MM-dd")))
            .arg(assignment.id() >= 0 ? RoleDisplay::iconAndLabel(assignment.role()) : QStringLiteral("(deleted assignment)"))
            .arg(user.id() >= 0 ? user.name() : QStringLiteral("(deleted member)"));
        auto *item = new QListWidgetItem(label, m_list);
        item->setData(Qt::UserRole, request.id());
    }
}

void NonAvailabilityRequestsView::selectionChanged()
{
    QListWidgetItem *selected = m_list->currentItem();
    if (!selected) {
        m_messageLabel->clear();
        return;
    }
    const int id = selected->data(Qt::UserRole).toInt();
    // listPending() was just used to populate the list, so re-fetching by
    // id here is the simplest way to get the full message without keeping
    // a parallel id->request map in sync.
    for (const NonAvailabilityRequest &request : m_requestController->listPending()) {
        if (request.id() == id) {
            m_messageLabel->setText(request.message());
            return;
        }
    }
}

void NonAvailabilityRequestsView::approveClicked()
{
    decide(true);
}

void NonAvailabilityRequestsView::denyClicked()
{
    decide(false);
}

void NonAvailabilityRequestsView::decide(bool approve)
{
    if (!m_isAdmin) {
        return;
    }
    QListWidgetItem *selected = m_list->currentItem();
    if (!selected) {
        QMessageBox::information(this, QStringLiteral("Decide Request"), QStringLiteral("Select a request first."));
        return;
    }
    const int id = selected->data(Qt::UserRole).toInt();
    if (m_requestController->decide(id, approve, m_adminUserId)) {
        refresh();
    } else {
        QMessageBox::critical(this, QStringLiteral("Decide Request"), m_requestController->lastError());
    }
}

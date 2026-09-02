#pragma once

#include <QWidget>

class QListWidget;
class QLabel;
class QPushButton;
class NonAvailabilityRequestController;
class AssignmentController;
class UserController;

// "Non-Availability Requests" tab (Admin-only,
// SCHEDULING_FUNCTIONAL_REQUIREMENTS.md FR-5): the approval queue for
// requests Members submit from the mobile app (FR-4). Pending requests
// only -- once decided they drop off this list (the Member sees the
// decision in their own "My Requests" screen on mobile).
class NonAvailabilityRequestsView : public QWidget
{
    Q_OBJECT

public:
    NonAvailabilityRequestsView(
        NonAvailabilityRequestController *requestController,
        AssignmentController *assignmentController,
        UserController *userController,
        QWidget *parent = nullptr);

public slots:
    void refresh();

    // Approve/Deny only make sense (and need a decidedByUserId) once an
    // Admin has unlocked via the title bar's lock icon -- there's no login
    // gate blocking the rest of the app, so the buttons simply don't
    // appear until then, matching DateNavigationTab's Assign Role button.
    void setAdminMode(bool isAdmin, int adminUserId);

private slots:
    void selectionChanged();
    void approveClicked();
    void denyClicked();

private:
    void decide(bool approve);

    NonAvailabilityRequestController *m_requestController = nullptr;
    AssignmentController *m_assignmentController = nullptr;
    UserController *m_userController = nullptr;
    bool m_isAdmin = false;
    int m_adminUserId = -1;

    QListWidget *m_list = nullptr;
    QLabel *m_messageLabel = nullptr;
    QPushButton *m_approveButton = nullptr;
    QPushButton *m_denyButton = nullptr;
};

#pragma once

#include <QMainWindow>
#include <QNetworkAccessManager>

#include "Controllers/AssignmentController.h"
#include "Controllers/CategoryController.h"
#include "Controllers/ItemController.h"
#include "Controllers/NonAvailabilityRequestController.h"
#include "Controllers/TagController.h"
#include "Controllers/UserController.h"
#include "Models/User.h"

class ItemListView;
class TagsCategoriesView;
class DateNavigationTab;
class NonAvailabilityRequestsView;
class TitleBar;
class QStackedWidget;

// Owns the controllers and wires the tabs to each other: e.g. creating a
// tag refreshes the tag list shown in both Items and Tags & Categories,
// and deciding a non-availability request refreshes the Date tab (where
// assignments are scheduled -- there is no separate Assignments tab).
// Adding an item happens via a modal dialog opened from the Items tab, not
// a separate tab. Frameless window: TitleBar (custom minimize/maximize/
// close + the tab bar) sits above a QStackedWidget that swaps between the
// views.
//
// There is no login gate at startup -- the app is usable read-only right
// away. Clicking the lock icon in the title bar opens a password-only
// LoginDialog; on success m_currentUser becomes that Admin and Admin-only
// actions (Assign Role/Edit/Delete on the Date tab, Approve/Deny on
// Non-Availability Requests) appear. Clicking the icon again logs out.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void toggleMaximizeRestore();
    void adminButtonClicked();

private:
    // Default-constructed (id() < 0, isAdmin() false) means logged out.
    User m_currentUser;

    ItemController m_itemController;
    TagController m_tagController;
    CategoryController m_categoryController;
    UserController m_userController;
    AssignmentController m_assignmentController;
    NonAvailabilityRequestController m_requestController;
    // Shared by DateNavigationTab for fetching Member avatars (DiceBear)
    // without blocking the UI thread.
    QNetworkAccessManager m_networkManager;

    ItemListView *m_itemListView = nullptr;
    TagsCategoriesView *m_tagsCategoriesView = nullptr;
    DateNavigationTab *m_dateNavigationTab = nullptr;
    NonAvailabilityRequestsView *m_requestsView = nullptr;

    TitleBar *m_titleBar = nullptr;
    QStackedWidget *m_stack = nullptr;
};

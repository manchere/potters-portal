#pragma once

#include <QMainWindow>
#include <QNetworkAccessManager>

#include "Controllers/AssignmentController.h"
#include "Controllers/CategoryController.h"
#include "Controllers/ItemController.h"
#include "Controllers/RoleTypeController.h"
#include "Controllers/SongController.h"
#include "Controllers/TagController.h"
#include "Controllers/UserController.h"
#include "Models/User.h"

class ItemListView;
class AdminOverviewView;
class DateNavigationTab;
class SongsView;
class ReportsView;
class TitleBar;
class QStackedWidget;

// Owns the controllers and wires the tabs to each other: e.g. creating a
// tag refreshes the tag list shown in both Items and the combined
// Taxonomy tab. Adding an item happens via a modal dialog opened from the
// Items tab, not a separate tab. Frameless window: TitleBar (custom
// minimize/maximize/close + the tab bar) sits above a QStackedWidget that
// swaps between the views.
//
// There is no login gate at startup -- the app is usable read-only right
// away. Clicking the lock icon in the title bar opens a password-only
// LoginDialog; on success m_currentUser becomes that Admin and Admin-only
// actions (Assign Role/Add Member/Edit/Delete on the Date tab, editing an
// assignment from the Taxonomy tab) appear. Clicking the icon again logs
// out.
//
// Note: the Non-Availability Requests tab (Admin approve/deny) has been
// removed from the desktop UI per product decision -- NonAvailabilityRequestsView
// and its backend (Controller/Model/REST endpoints, used by the mobile
// submission flow) are left intact, just unwired from MainWindow, in case
// the desktop approval UI comes back later.
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
    void themeButtonClicked();

private:
    // Default-constructed (id() < 0, isAdmin() false) means logged out.
    User m_currentUser;

    ItemController m_itemController;
    TagController m_tagController;
    CategoryController m_categoryController;
    UserController m_userController;
    AssignmentController m_assignmentController;
    RoleTypeController m_roleTypeController;
    SongController m_songController;
    // Shared by DateNavigationTab for fetching Member avatars (DiceBear)
    // without blocking the UI thread.
    QNetworkAccessManager m_networkManager;

    ItemListView *m_itemListView = nullptr;
    AdminOverviewView *m_adminOverviewView = nullptr;
    DateNavigationTab *m_dateNavigationTab = nullptr;
    SongsView *m_songsView = nullptr;
    ReportsView *m_reportsView = nullptr;

    TitleBar *m_titleBar = nullptr;
    QStackedWidget *m_stack = nullptr;
};

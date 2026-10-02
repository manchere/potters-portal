#pragma once

#include <QMainWindow>

#include "Controllers/DutyController.h"
#include "Controllers/CategoryController.h"
#include "Controllers/ItemController.h"
#include "Controllers/DutyTypeController.h"
#include "Controllers/TeamController.h"
#include "Controllers/SongController.h"
#include "Controllers/TagController.h"
#include "Controllers/UserController.h"
#include "Models/User.h"
#include "Style.h"

class ItemListView;
class AdminOverviewView;
class ScheduleTab;
class SettingsView;
class SongsView;
class ReportsView;
class Sidebar;
class TitleBar;
class QStackedWidget;

// Owns the controllers and wires the tabs to each other: e.g. creating a
// tag refreshes the tag list shown in both Items and the combined
// Taxonomy tab. Adding an item happens via a modal dialog opened from the
// Inventory tab, not a separate tab. Frameless window: TitleBar (custom
// minimize/maximize/close + the admin lock) sits above the Sidebar and a
// QStackedWidget that swaps between the pages; the last page, Settings,
// holds the theme choice and the Admin password change.
//
// There is no login gate at startup -- the app is usable read-only right
// away. Clicking the lock icon in the title bar opens a password-only
// LoginDialog; on success m_currentUser becomes that Admin and Admin-only
// actions (Assign Duty/Add Member/Edit/Delete on the Schedule tab, editing a
// duty from the Taxonomy tab) appear. Clicking the icon again logs
// out.
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
    void changePasswordClicked();
    void themeChosen(Theme theme);

private:
    // Default-constructed (id() < 0, isAdmin() false) means logged out.
    User m_currentUser;

    ItemController m_itemController;
    TagController m_tagController;
    CategoryController m_categoryController;
    UserController m_userController;
    DutyController m_dutyController;
    DutyTypeController m_dutyTypeController;
    TeamController m_teamController;
    SongController m_songController;

    ItemListView *m_itemListView = nullptr;
    AdminOverviewView *m_adminOverviewView = nullptr;
    ScheduleTab *m_scheduleTab = nullptr;
    SongsView *m_songsView = nullptr;
    ReportsView *m_reportsView = nullptr;
    SettingsView *m_settingsView = nullptr;

    TitleBar *m_titleBar = nullptr;
    Sidebar *m_sidebar = nullptr;
    QStackedWidget *m_stack = nullptr;
};

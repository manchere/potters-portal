#pragma once

#include <QMainWindow>

#include "Controllers/AccessController.h"
#include "Controllers/DutyController.h"
#include "Controllers/CategoryController.h"
#include "Controllers/ItemController.h"
#include "Controllers/DutyTypeController.h"
#include "Controllers/FeedbackController.h"
#include "Controllers/TeamController.h"
#include "Controllers/SongController.h"
#include "Controllers/TagController.h"
#include "Controllers/UserController.h"
#include "Models/User.h"
#include "Language.h"
#include "Style.h"

class ItemListView;
class AdminOverviewView;
class ScheduleTab;
class SettingsView;
class SongsView;
class FeedbackView;
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
// There is no login gate at startup -- the app opens with whatever
// Settings > Access Rights allows everyone. Clicking the lock icon in the
// title bar opens LoginDialog (email + password) for any member; once
// signed in, applyAccess() works out what they may do (AccessController)
// and hides tabs and buttons they can't use. Admins get everything,
// including the Schedule tab's actions. Clicking the icon again signs out.
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
    void accessRightsClicked();
    // Works out the signed-in person's rights (or everyone's) and applies
    // them to the sidebar and every tab. Re-run when rules, duties, or
    // members change, since a duty's rights depend on the upcoming Sunday.
    void applyAccess();
    void changePasswordClicked();
    void themeChosen(Theme theme);
    // Saves the language and offers to restart into it.
    void languageChosen(Language language);

private:
    // Default-constructed (id() < 0, isAdmin() false) means signed out.
    User m_currentUser;

    ItemController m_itemController;
    TagController m_tagController;
    CategoryController m_categoryController;
    UserController m_userController;
    DutyController m_dutyController;
    DutyTypeController m_dutyTypeController;
    TeamController m_teamController;
    SongController m_songController;
    FeedbackController m_feedbackController;
    AccessController m_accessController;

    ItemListView *m_itemListView = nullptr;
    AdminOverviewView *m_adminOverviewView = nullptr;
    ScheduleTab *m_scheduleTab = nullptr;
    SongsView *m_songsView = nullptr;
    FeedbackView *m_feedbackView = nullptr;
    ReportsView *m_reportsView = nullptr;
    SettingsView *m_settingsView = nullptr;

    TitleBar *m_titleBar = nullptr;
    Sidebar *m_sidebar = nullptr;
    QStackedWidget *m_stack = nullptr;
};

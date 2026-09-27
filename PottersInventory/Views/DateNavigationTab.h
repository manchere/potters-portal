#pragma once

#include <QDate>
#include <QSet>
#include <QWidget>

class QLabel;
class QListWidget;
class QListWidgetItem;
class QNetworkAccessManager;
class QPushButton;
class AssignmentController;
class UserController;
class RoleTypeController;
class Assignment;

// Leftmost tab (SCHEDULING_FUNCTIONAL_REQUIREMENTS.md FR-6/FR-7/FR-8): pick
// a Sunday from a scrollable list (e.g. "Sun 6 Sep 2026"), see every
// Member's role assignment (with avatar + role icon) for it, and
// create/edit/delete assignments right here via the "Assign Role" button
// -- there is no separate Assignments tab. "+ Add Member" creates a new
// Member profile without leaving this tab either.
//
// "Copy Schedule" / "Paste Schedule" (or Ctrl+C / Ctrl+V) copy one
// Sunday's whole schedule onto another -- e.g. reuse last month's
// Communion Sunday line-up -- via AssignmentController::copySchedule.
class DateNavigationTab : public QWidget
{
    Q_OBJECT

public:
    DateNavigationTab(
        AssignmentController *assignmentController,
        UserController *userController,
        RoleTypeController *roleTypeController,
        QNetworkAccessManager *networkManager,
        QWidget *parent = nullptr);

public slots:
    void refresh();

    // Shows/hides the Assign Role / Add Member / Edit / Delete buttons --
    // scheduling and profile creation are Admin-only actions, and there's
    // no login gate blocking the rest of the app, so these simply don't
    // appear until an Admin unlocks via the title bar's lock icon.
    void setAdminMode(bool isAdmin);

private slots:
    void sundaySelectionChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void assignClicked();
    void assignForSelectedMemberClicked();
    void addMemberClicked();
    void editClicked();
    void deleteClicked();
    void memberDoubleClicked(QListWidgetItem *item);
    void copyScheduleClicked();
    void pasteScheduleClicked();

private:
    void rebuildResults();
    void populateSundayList();
    void selectSunday(const QDate &date);
    // Applies the right look to a Sunday-list row: a distinct highlight
    // when it's the selected date, otherwise the gold "has assignments"
    // tint (if any) or the plain default.
    void applySundayItemStyle(QListWidgetItem *item, bool isSelected) const;
    QWidget *buildRow(const Assignment &assignment);
    QString memberName(int userId) const;
    QString memberAvatarSeed(int userId) const;
    // Assignments only ever happen on Sundays -- rounds forward to the
    // Sunday of date's week (or date itself, if it's already Sunday).
    static QDate nearestSunday(const QDate &date);
    // Enables Copy/Paste for the selected Sunday and updates the
    // "Copied: ..." label.
    void updateCopyPasteState();

    AssignmentController *m_assignmentController = nullptr;
    UserController *m_userController = nullptr;
    RoleTypeController *m_roleTypeController = nullptr;
    QNetworkAccessManager *m_networkManager = nullptr;

    QListWidget *m_sundayList = nullptr;
    QListWidget *m_resultsList = nullptr;
    QPushButton *m_assignButton = nullptr;
    QPushButton *m_addMemberButton = nullptr;
    // Top-right: prefills the Member combo with whichever row is
    // currently selected, for quickly giving that same person another
    // role on this date (FR-7.2) -- enabled only when a row is selected.
    QPushButton *m_assignForMemberButton = nullptr;
    QPushButton *m_editButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QPushButton *m_copyButton = nullptr;
    QPushButton *m_pasteButton = nullptr;
    QLabel *m_copiedLabel = nullptr;

    bool m_isAdmin = false;
    QDate m_selectedDate;
    QSet<QDate> m_datesWithAssignments;
    // -1 when no row is selected.
    int m_selectedAssignmentId = -1;
    // The Sunday whose schedule was last copied; invalid until Copy is
    // used. Only the date is kept -- the paste reads whatever is on that
    // Sunday at paste time.
    QDate m_copiedDate;
};

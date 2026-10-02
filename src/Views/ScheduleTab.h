#pragma once

#include <QDate>
#include <QHash>
#include <QSet>
#include <QWidget>

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class DutyController;
class UserController;
class DutyTypeController;
class Duty;

// Leftmost tab (SCHEDULING_FUNCTIONAL_REQUIREMENTS.md FR-6/FR-7/FR-8): pick
// a Sunday from a scrollable list (e.g. "Sun 6 Sep 2026"), see every
// Member's duty (with color badge + duty icon) for it, and
// create/edit/delete duties right here via the "Assign Duty" button
// -- there is no separate Duties tab. "+ Add Member" puts a Member on the
// open Sunday's schedule: their name, one or more duties, and a backup
// (see AddToScheduleDialog). New Member profiles are made on Taxonomy.
//
// A Sunday that has passed is read-only: nothing on it can be assigned,
// edited, deleted, or pasted over (DutyController enforces the same), though
// it can still be copied onto an upcoming Sunday.
//
// "Copy Schedule" / "Paste Schedule" (or Ctrl+C / Ctrl+V) copy one
// Sunday's whole schedule onto another -- e.g. reuse last month's
// Communion Sunday line-up -- via DutyController::copySchedule.
class ScheduleTab : public QWidget
{
    Q_OBJECT

public:
    ScheduleTab(
        DutyController *dutyController,
        UserController *userController,
        DutyTypeController *dutyTypeController,
        QWidget *parent = nullptr);

public slots:
    void refresh();

    // Shows/hides the Assign Duty / Add Member / Edit / Delete buttons --
    // scheduling and profile creation are Admin-only actions, and there's
    // no login gate blocking the rest of the app, so these simply don't
    // appear until an Admin unlocks via the title bar's lock icon.
    void setAdminMode(bool isAdmin);

    // Recolors the Sunday rows for the current theme (see
    // applySundayItemStyle); called after the theme is switched.
    void restyleSundayItems();

private slots:
    void sundaySelectionChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void sundaySearchChanged();
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
    // Hides Sunday rows that don't match the search box. Every word typed
    // must appear in the date or in one single duty on that Sunday
    // (its duty, member or support member), so "usher grace" finds the
    // Sundays Grace ushers, not ones where she merely serves.
    void applySundayFilter();
    void selectSunday(const QDate &date);
    // Applies the right look to a Sunday-list row: a distinct highlight
    // when it's the selected date, otherwise the gold "has duties"
    // tint (if any) or the plain default.
    void applySundayItemStyle(QListWidgetItem *item, bool isSelected) const;
    QWidget *buildRow(const Duty &duty);
    QString memberName(int userId) const;
    // Duties only ever happen on Sundays -- rounds forward to the
    // Sunday of date's week (or date itself, if it's already Sunday).
    static QDate nearestSunday(const QDate &date);
    // Enables Copy/Paste for the selected Sunday and updates the
    // "Copied: ..." label.
    void updateCopyPasteState();
    // Enables/disables every action for the current login, selected
    // Sunday (past ones are read-only), and selected duty.
    void updateActionState();
    bool selectedSundayEditable() const;

    DutyController *m_dutyController = nullptr;
    UserController *m_userController = nullptr;
    DutyTypeController *m_dutyTypeController = nullptr;

    QLineEdit *m_sundaySearch = nullptr;
    QLabel *m_noSundayMatchLabel = nullptr;
    QListWidget *m_sundayList = nullptr;
    QListWidget *m_resultsList = nullptr;
    QPushButton *m_assignButton = nullptr;
    QPushButton *m_addMemberButton = nullptr;
    // Top-right: prefills the Member combo with whichever row is
    // currently selected, for quickly giving that same person another
    // duty on this date (FR-7.2) -- enabled only when a row is selected.
    QPushButton *m_assignForMemberButton = nullptr;
    QPushButton *m_editButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QPushButton *m_copyButton = nullptr;
    QPushButton *m_pasteButton = nullptr;
    QLabel *m_copiedLabel = nullptr;
    QLabel *m_pastNotice = nullptr;

    bool m_isAdmin = false;
    QDate m_selectedDate;
    QSet<QDate> m_datesWithDuties;
    // Lower-cased text the search box matches against, one entry per
    // duty on that Sunday (each includes the date's own spellings),
    // or just the date text when it has none. Built in populateSundayList
    // so typing doesn't hit the database.
    QHash<QDate, QStringList> m_sundaySearchText;
    // -1 when no row is selected.
    int m_selectedDutyId = -1;
    // The Sunday whose schedule was last copied; invalid until Copy is
    // used. Only the date is kept -- the paste reads whatever is on that
    // Sunday at paste time.
    QDate m_copiedDate;
};

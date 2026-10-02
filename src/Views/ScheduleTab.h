#pragma once

#include <QDate>
#include <QHash>
#include <QPointer>
#include <QSet>
#include <QVector>
#include <QWidget>

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QToolButton;
class DutyController;
class UserController;
class DutyTypeController;
class TeamController;
class Duty;
class User;
class QCheckBox;

// Leftmost tab (SCHEDULING_FUNCTIONAL_REQUIREMENTS.md FR-6/FR-7/FR-8): pick
// a Sunday from a scrollable list (e.g. "Sun 6 Sep 2026"), see every
// Member's duty (with color badge + duty icon) for it, and
// create/edit/delete duties right here via the "Assign Duty" button
// -- there is no separate Duties tab. New Member profiles are made on
// Taxonomy.
//
// The Sunday list shows past Sundays that had a schedule (orange), then
// every Sunday from the upcoming one on: the upcoming one green once it has
// duties, later ones gold once they do.
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
        TeamController *teamController,
        QWidget *parent = nullptr);

public slots:
    void refresh();

    // Shows/hides the Assign Duty / Edit / Delete buttons --
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
    // when it's the selected date; otherwise orange for a past Sunday,
    // green for the upcoming one once it has duties, gold for a later one
    // with duties, or the plain default.
    void applySundayItemStyle(QListWidgetItem *item, bool isSelected) const;
    QWidget *buildRow(const Duty &duty);
    // The member's name (and any notes) beside their badge. All rows' name
    // columns share one width (see fitMemberColumns), so duties line up.
    QWidget *buildMemberColumn(const QString &name, const QString &nameStyle, const QString &notes, QWidget *parent);
    // Fits the rows to the list's width: every name column gets the same
    // width (the longest name on this Sunday when there's room, less --
    // names end in "..." -- when not), and when even the shortest name
    // column leaves too little room, each duty's backup moves under its
    // duty tag. The duty tag and backup always stay in view.
    void fitMemberColumns();
    // A duty's tag and backup, side by side or (stacked) one above the
    // other; registered so fitMemberColumns can switch them.
    QWidget *buildDutyContent(const Duty &duty, QWidget *parent);
    void setRowsStacked(bool stacked);
    // Re-measures each list row's height after its layout changed.
    void updateRowHeights();
    // After the rows are built: measures them and sets the list's minimum
    // width, then fits the name columns.
    void finishRows();
    // "Combine each member's duties" view: one row for a member, with a
    // line per duty showing that duty's own backup and, for an Admin on
    // an upcoming Sunday, an Edit button for just that duty.
    QWidget *buildMemberRow(const User &member, const QVector<Duty> &duties);
    // "Backup" + small badge + the duty's backup's name, or a dash.
    QWidget *buildBackupLine(const Duty &duty, QWidget *parent);
    QWidget *buildDutyCell(const Duty &duty, QWidget *parent);
    QToolButton *buildEditButton(int dutyId, QWidget *parent);
    // Opens Edit Duty for one duty and saves it.
    void editDuty(int dutyId);
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
    // Opens "Edit Member on Schedule" for memberId's duties on the selected
    // Sunday and applies the changes.
    void editMemberOnSchedule(int memberId);
    // Refits the name columns when the list is resized.
    bool eventFilter(QObject *watched, QEvent *event) override;

    DutyController *m_dutyController = nullptr;
    UserController *m_userController = nullptr;
    DutyTypeController *m_dutyTypeController = nullptr;
    TeamController *m_teamController = nullptr;

    QLineEdit *m_sundaySearch = nullptr;
    QLabel *m_noSundayMatchLabel = nullptr;
    QListWidget *m_sundayList = nullptr;
    QListWidget *m_resultsList = nullptr;
    QCheckBox *m_combineCheck = nullptr;
    QPushButton *m_assignButton = nullptr;
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
    // -1 when no row is selected. On a combined member row it's that
    // member's first duty, and m_selectedMemberId is the member (-1 on
    // a single-duty row), so Edit/Delete act on all their duties.
    int m_selectedDutyId = -1;
    int m_selectedMemberId = -1;
    // Width of the widest duty pill on the open Sunday (rebuildResults).
    int m_dutyColumnWidth = 0;
    // Name columns of the rows now shown, the width they'd like (longest
    // name/notes, capped), and how wide a row is apart from its name column.
    QVector<QPointer<QWidget>> m_memberColumns;
    int m_nameColumnWidth = 0;
    int m_rowFixedWidth = 0;
    // The same, with every backup under its duty tag.
    int m_rowFixedWidthStacked = 0;
    QVector<QPointer<QWidget>> m_dutyContents;
    bool m_rowsStacked = false;
    bool m_fitting = false;
    // The Sunday whose schedule was last copied; invalid until Copy is
    // used. Only the date is kept -- the paste reads whatever is on that
    // Sunday at paste time.
    QDate m_copiedDate;
};

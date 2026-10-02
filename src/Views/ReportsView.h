#pragma once

#include <QDate>
#include <QVector>
#include <QWidget>

#include "Models/ScheduleReportRow.h"

class QDateEdit;
class QLabel;
class QListWidget;
class QPushButton;
class QTextBrowser;
class DutyController;
class DutyTypeController;
class TeamController;
class UserController;
class MemberPickerField;

// "Reports" tab -- look up past Sunday schedules. Pick a date range (and
// optionally some Members); the left list shows every Sunday in that range
// that had a schedule, plus an "All Sundays in range" entry at the top.
// The right side renders a read-only "who did what" report:
//   - a single Sunday: its line-up (duty, who served, backup, notes). A
//     small tag appears next to a name only when that person had asked
//     for time off or marked themselves away that day;
//   - all Sundays in range: each Sunday's line-up in turn, newest first.
// "Save Report..." writes the current report as HTML (printable from any
// browser) or CSV (for a spreadsheet).
//
// Open to everyone, like the Schedule tab's read-only view. Time-off request
// reasons are never shown -- only their status.
class ReportsView : public QWidget
{
    Q_OBJECT

public:
    ReportsView(DutyController *dutyController, UserController *userController,
                DutyTypeController *dutyTypeController, TeamController *teamController,
                QWidget *parent = nullptr);

public slots:
    // Reloads the Member filter and the report (e.g. after duties or
    // users change).
    void refresh();

    // Save Report follows the Reports "Create" right in Settings >
    // Access Rights.
    void setCanSave(bool canSave);

    // Re-renders the on-screen report in the current theme's colors;
    // called after the theme is switched.
    void restyleReport();

private slots:
    void reloadReport();
    void selectionChanged();
    void presetClicked(int months);
    void saveClicked();

private:
    // Fills the Members, Duties and Teams pickers.
    void populateFilters();
    void populateSundayList();
    // darkColors is for the on-screen view in the dark themes only; saved
    // files always use the light colors so they print well.
    QString rangeHtml(bool darkColors) const;
    QString sundayHtml(const QDate &date, bool darkColors) const;
    QString wrapHtml(const QString &title, const QString &subtitle, const QString &body, bool darkColors) const;
    // Title, subtitle, and body for whichever entry is selected.
    QString currentReportHtml(bool darkColors) const;
    // Rows the current view shows: the whole range for "All Sundays", or
    // just the selected Sunday.
    QVector<ScheduleReportRow> visibleRows() const;
    // Invalid when "All Sundays in range" is selected.
    QDate selectedSunday() const;
    QString filterDescription() const;
    // The chosen members, duties and teams, e.g. "Grace · Singing · Choir";
    // empty when nothing is picked.
    QString selectionDescription() const;

    DutyController *m_dutyController = nullptr;
    UserController *m_userController = nullptr;
    DutyTypeController *m_dutyTypeController = nullptr;
    TeamController *m_teamController = nullptr;

    // Cached result of the last scheduleReport() query for the current
    // range + member filter.
    QVector<ScheduleReportRow> m_rows;

    QDateEdit *m_fromEdit = nullptr;
    QDateEdit *m_toEdit = nullptr;
    MemberPickerField *m_memberField = nullptr;
    MemberPickerField *m_dutyField = nullptr;
    MemberPickerField *m_teamField = nullptr;
    QListWidget *m_sundayList = nullptr;
    QTextBrowser *m_reportView = nullptr;
    QPushButton *m_saveButton = nullptr;
    bool m_canSave = false;
    QLabel *m_statusLabel = nullptr;
};

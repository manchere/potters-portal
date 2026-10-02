#pragma once

#include <QDate>
#include <QVector>
#include <QWidget>

#include "Models/ScheduleReportRow.h"

class QComboBox;
class QDateEdit;
class QLabel;
class QListWidget;
class QPushButton;
class QTextBrowser;
class AssignmentController;
class UserController;

// "Reports" tab -- look up past Sunday schedules. Pick a date range (and
// optionally one Member); the left list shows every Sunday in that range
// that had a schedule, plus an "All Sundays in range" entry at the top.
// The right side renders a read-only "who did what" report:
//   - a single Sunday: its line-up (role, who served, backup, notes). A
//     small tag appears next to a name only when that person had asked
//     for time off or marked themselves away that day;
//   - all Sundays in range: each Sunday's line-up in turn, newest first.
// "Save Report..." writes the current report as HTML (printable from any
// browser) or CSV (for a spreadsheet).
//
// Open to everyone, like the Date tab's read-only view. Time-off request
// reasons are never shown -- only their status.
class ReportsView : public QWidget
{
    Q_OBJECT

public:
    ReportsView(AssignmentController *assignmentController, UserController *userController, QWidget *parent = nullptr);

public slots:
    // Reloads the Member filter and the report (e.g. after assignments or
    // users change).
    void refresh();

    // Re-renders the on-screen report in the current theme's colors;
    // called after the theme is switched.
    void restyleReport();

private slots:
    void reloadReport();
    void selectionChanged();
    void presetClicked(int months);
    void saveClicked();

private:
    void populateMemberFilter();
    void populateSundayList();
    // darkColors is for the on-screen view in the black theme only; saved
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

    AssignmentController *m_assignmentController = nullptr;
    UserController *m_userController = nullptr;

    // Cached result of the last scheduleReport() query for the current
    // range + member filter.
    QVector<ScheduleReportRow> m_rows;

    QDateEdit *m_fromEdit = nullptr;
    QDateEdit *m_toEdit = nullptr;
    QComboBox *m_memberCombo = nullptr;
    QListWidget *m_sundayList = nullptr;
    QTextBrowser *m_reportView = nullptr;
    QPushButton *m_saveButton = nullptr;
    QLabel *m_statusLabel = nullptr;
};

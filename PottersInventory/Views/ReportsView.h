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
// that had a schedule, plus a "Summary" entry at the top. The right side
// renders a read-only report:
//   - a single Sunday: each role, who served, support, notes, and whether
//     the member had asked for time off or marked themselves unavailable;
//   - the summary: Sundays covered, unfilled slots, time-off requests, and
//     how often each Member and role served.
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

private slots:
    void reloadReport();
    void selectionChanged();
    void presetClicked(int months);
    void saveClicked();

private:
    void populateMemberFilter();
    void populateSundayList();
    QString summaryHtml() const;
    QString sundayHtml(const QDate &date) const;
    QString wrapHtml(const QString &title, const QString &body) const;
    // Rows the current view shows: the whole range for the summary, or
    // just the selected Sunday.
    QVector<ScheduleReportRow> visibleRows() const;
    // Invalid when the summary entry is selected.
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

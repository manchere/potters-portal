#pragma once

#include <QWidget>

class QCalendarWidget;
class QListWidget;
class QListWidgetItem;
class QNetworkAccessManager;
class QPushButton;
class AssignmentController;
class UserController;
class Assignment;

// Leftmost tab (SCHEDULING_FUNCTIONAL_REQUIREMENTS.md FR-6/FR-7/FR-8): pick
// a date, see every Member's role assignment (with avatar + role icon) for
// that Sunday, and create/edit/delete assignments right here via the
// "Assign Role" button -- there is no separate Assignments tab.
class DateNavigationTab : public QWidget
{
    Q_OBJECT

public:
    DateNavigationTab(
        AssignmentController *assignmentController,
        UserController *userController,
        QNetworkAccessManager *networkManager,
        QWidget *parent = nullptr);

public slots:
    void refresh();

    // Shows/hides the Assign Role / Edit / Delete buttons -- scheduling is
    // an Admin-only action, and there's no login gate blocking the rest of
    // the app, so these simply don't appear until an Admin unlocks via the
    // title bar's lock icon.
    void setAdminMode(bool isAdmin);

private slots:
    void dateSelected(const QDate &date);
    void assignClicked();
    void editClicked();
    void deleteClicked();
    void memberDoubleClicked(QListWidgetItem *item);

private:
    void rebuildResults();
    void updateCalendarHighlights();
    QWidget *buildRow(const Assignment &assignment);
    QString memberName(int userId) const;
    QString memberAvatarSeed(int userId) const;

    AssignmentController *m_assignmentController = nullptr;
    UserController *m_userController = nullptr;
    QNetworkAccessManager *m_networkManager = nullptr;

    QCalendarWidget *m_calendar = nullptr;
    QListWidget *m_resultsList = nullptr;
    QPushButton *m_assignButton = nullptr;
    QPushButton *m_editButton = nullptr;
    QPushButton *m_deleteButton = nullptr;

    bool m_isAdmin = false;
    // -1 when no row is selected.
    int m_selectedAssignmentId = -1;
};

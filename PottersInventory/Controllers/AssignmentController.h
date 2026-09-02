#pragma once

#include <QDate>
#include <QObject>
#include <QVector>

#include "Models/Assignment.h"

// Backed by Postgres (assignments table). See ItemController for the
// query pattern.
class AssignmentController : public QObject
{
    Q_OBJECT

public:
    explicit AssignmentController(QObject *parent = nullptr);

    QVector<Assignment> allAssignments() const;
    Assignment assignmentById(int id) const;

    // Assignments where userId is either the primary or support member,
    // from today onward, soonest first (FR-2.1's "upcoming Sundays").
    QVector<Assignment> assignmentsForMember(int userId) const;

    // Every assignment (past and upcoming) where userId is either the
    // primary or support member, most recent first -- used by the desktop
    // Member stats modal, unlike assignmentsForMember() which is
    // upcoming-only for the mobile "my assignments" API.
    QVector<Assignment> allAssignmentsForMember(int userId) const;

    // Every assignment on a specific date, for the desktop Date tab
    // (FR-8.1/8.2).
    QVector<Assignment> assignmentsForDate(const QDate &date) const;

    QString lastError() const { return m_lastError; }

public slots:
    bool addAssignment(Assignment &assignment);
    bool updateAssignment(const Assignment &assignment);
    bool removeAssignment(int id);

signals:
    void assignmentsChanged();

private:
    mutable QString m_lastError;
};

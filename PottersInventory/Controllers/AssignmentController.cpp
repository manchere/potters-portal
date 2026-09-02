#include "AssignmentController.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "Database/Database.h"

namespace
{
    const QString kSelectColumns = QStringLiteral(
        "id, role, service_date, member_id, support_member_id, notes");
}

AssignmentController::AssignmentController(QObject *parent)
    : QObject(parent)
{
}

static Assignment assignmentFromQuery(const QSqlQuery &query)
{
    Assignment assignment;
    assignment.setId(query.value(QStringLiteral("id")).toInt());
    assignment.setRole(assignmentRoleFromString(query.value(QStringLiteral("role")).toString()));
    assignment.setServiceDate(query.value(QStringLiteral("service_date")).toDate());
    const QVariant memberId = query.value(QStringLiteral("member_id"));
    assignment.setMemberId(memberId.isNull() ? -1 : memberId.toInt());
    const QVariant supportMemberId = query.value(QStringLiteral("support_member_id"));
    assignment.setSupportMemberId(supportMemberId.isNull() ? -1 : supportMemberId.toInt());
    assignment.setNotes(query.value(QStringLiteral("notes")).toString());
    return assignment;
}

QVector<Assignment> AssignmentController::allAssignments() const
{
    QVector<Assignment> assignments;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT %1 FROM assignments ORDER BY service_date").arg(kSelectColumns));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return assignments;
    }
    while (query.next()) {
        assignments.append(assignmentFromQuery(query));
    }
    return assignments;
}

Assignment AssignmentController::assignmentById(int id) const
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT %1 FROM assignments WHERE id = :id").arg(kSelectColumns));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return Assignment();
    }
    return assignmentFromQuery(query);
}

QVector<Assignment> AssignmentController::assignmentsForMember(int userId) const
{
    QVector<Assignment> assignments;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT %1 FROM assignments "
        "WHERE (member_id = :user_id OR support_member_id = :user_id) AND service_date >= CURRENT_DATE "
        "ORDER BY service_date").arg(kSelectColumns));
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return assignments;
    }
    while (query.next()) {
        assignments.append(assignmentFromQuery(query));
    }
    return assignments;
}

QVector<Assignment> AssignmentController::allAssignmentsForMember(int userId) const
{
    QVector<Assignment> assignments;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT %1 FROM assignments "
        "WHERE member_id = :user_id OR support_member_id = :user_id "
        "ORDER BY service_date DESC").arg(kSelectColumns));
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return assignments;
    }
    while (query.next()) {
        assignments.append(assignmentFromQuery(query));
    }
    return assignments;
}

QVector<Assignment> AssignmentController::assignmentsForDate(const QDate &date) const
{
    QVector<Assignment> assignments;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT %1 FROM assignments WHERE service_date = :service_date ORDER BY role").arg(kSelectColumns));
    query.bindValue(QStringLiteral(":service_date"), date);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return assignments;
    }
    while (query.next()) {
        assignments.append(assignmentFromQuery(query));
    }
    return assignments;
}

bool AssignmentController::addAssignment(Assignment &assignment)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT INTO assignments (role, service_date, member_id, support_member_id, notes) "
        "VALUES (:role, :service_date, :member_id, :support_member_id, :notes) RETURNING id"));
    query.bindValue(QStringLiteral(":role"), assignmentRoleToString(assignment.role()));
    query.bindValue(QStringLiteral(":service_date"), assignment.serviceDate());
    query.bindValue(QStringLiteral(":member_id"),
        assignment.memberId() > 0 ? QVariant(assignment.memberId()) : QVariant());
    query.bindValue(QStringLiteral(":support_member_id"),
        assignment.supportMemberId() > 0 ? QVariant(assignment.supportMemberId()) : QVariant());
    query.bindValue(QStringLiteral(":notes"), assignment.notes());
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return false;
    }
    assignment.setId(query.value(0).toInt());
    emit assignmentsChanged();
    return true;
}

bool AssignmentController::updateAssignment(const Assignment &assignment)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "UPDATE assignments SET role = :role, service_date = :service_date, member_id = :member_id, "
        "support_member_id = :support_member_id, notes = :notes, updated_at = now() WHERE id = :id"));
    query.bindValue(QStringLiteral(":role"), assignmentRoleToString(assignment.role()));
    query.bindValue(QStringLiteral(":service_date"), assignment.serviceDate());
    query.bindValue(QStringLiteral(":member_id"),
        assignment.memberId() > 0 ? QVariant(assignment.memberId()) : QVariant());
    query.bindValue(QStringLiteral(":support_member_id"),
        assignment.supportMemberId() > 0 ? QVariant(assignment.supportMemberId()) : QVariant());
    query.bindValue(QStringLiteral(":notes"), assignment.notes());
    query.bindValue(QStringLiteral(":id"), assignment.id());
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    emit assignmentsChanged();
    return true;
}

bool AssignmentController::removeAssignment(int id)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM assignments WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    emit assignmentsChanged();
    return true;
}

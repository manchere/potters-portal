#include "AssignmentController.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "Database/Database.h"

namespace
{
    const QString kSelectColumns = QStringLiteral(
        "id, role_id, service_date, member_id, support_member_id, notes");
}

AssignmentController::AssignmentController(QObject *parent)
    : QObject(parent)
{
}

static Assignment assignmentFromQuery(const QSqlQuery &query)
{
    Assignment assignment;
    assignment.setId(query.value(QStringLiteral("id")).toInt());
    assignment.setRoleId(query.value(QStringLiteral("role_id")).toInt());
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
        "SELECT %1 FROM assignments WHERE service_date = :service_date ORDER BY role_id").arg(kSelectColumns));
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
        "INSERT INTO assignments (role_id, service_date, member_id, support_member_id, notes) "
        "VALUES (:role_id, :service_date, :member_id, :support_member_id, :notes) RETURNING id"));
    query.bindValue(QStringLiteral(":role_id"), assignment.roleId());
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
        "UPDATE assignments SET role_id = :role_id, service_date = :service_date, member_id = :member_id, "
        "support_member_id = :support_member_id, notes = :notes, updated_at = now() WHERE id = :id"));
    query.bindValue(QStringLiteral(":role_id"), assignment.roleId());
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

QStringList AssignmentController::membersMarkedUnavailable(const QDate &date) const
{
    QStringList names;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT DISTINCT u.name FROM assignments a "
        "JOIN users u ON u.id = a.member_id OR u.id = a.support_member_id "
        "JOIN availability_marks m ON m.user_id = u.id AND m.date = a.service_date "
        "WHERE a.service_date = :service_date ORDER BY u.name"));
    query.bindValue(QStringLiteral(":service_date"), date);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return names;
    }
    while (query.next()) {
        names.append(query.value(0).toString());
    }
    return names;
}

bool AssignmentController::copySchedule(
    const QDate &fromDate, const QDate &toDate, bool replaceExisting, int *copied, int *skipped)
{
    if (copied) {
        *copied = 0;
    }
    if (skipped) {
        *skipped = 0;
    }
    if (fromDate == toDate) {
        m_lastError = QStringLiteral("Pick a different Sunday to paste onto.");
        return false;
    }

    Database::ensureConnected();
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.transaction()) {
        m_lastError = db.lastError().text();
        return false;
    }
    auto fail = [this, &db](const QSqlQuery &query) {
        m_lastError = query.lastError().text();
        db.rollback();
        return false;
    };

    QSqlQuery countSource;
    countSource.prepare(QStringLiteral("SELECT COUNT(*) FROM assignments WHERE service_date = :from_date"));
    countSource.bindValue(QStringLiteral(":from_date"), fromDate);
    if (!countSource.exec() || !countSource.next()) {
        return fail(countSource);
    }
    const int sourceCount = countSource.value(0).toInt();

    if (replaceExisting) {
        QSqlQuery clear;
        clear.prepare(QStringLiteral("DELETE FROM assignments WHERE service_date = :to_date"));
        clear.bindValue(QStringLiteral(":to_date"), toDate);
        if (!clear.exec()) {
            return fail(clear);
        }
    }

    // IS NOT DISTINCT FROM so an unassigned (NULL member) slot for a role
    // also counts as "already there" and isn't duplicated.
    QSqlQuery insert;
    insert.prepare(QStringLiteral(
        "INSERT INTO assignments (role_id, service_date, member_id, support_member_id, notes) "
        "SELECT src.role_id, CAST(:to_date AS DATE), src.member_id, src.support_member_id, src.notes "
        "FROM assignments src "
        "WHERE src.service_date = :from_date "
        "AND NOT EXISTS (SELECT 1 FROM assignments dst WHERE dst.service_date = :to_date "
        "AND dst.role_id = src.role_id AND dst.member_id IS NOT DISTINCT FROM src.member_id) "
        "ORDER BY src.id"));
    insert.bindValue(QStringLiteral(":to_date"), toDate);
    insert.bindValue(QStringLiteral(":from_date"), fromDate);
    if (!insert.exec()) {
        return fail(insert);
    }
    const int inserted = insert.numRowsAffected();

    if (!db.commit()) {
        m_lastError = db.lastError().text();
        db.rollback();
        return false;
    }
    if (copied) {
        *copied = inserted;
    }
    if (skipped) {
        *skipped = sourceCount - inserted;
    }
    emit assignmentsChanged();
    return true;
}

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

QVector<ScheduleReportRow> AssignmentController::scheduleReport(const QDate &fromDate, const QDate &toDate, int memberId) const
{
    QVector<ScheduleReportRow> rows;
    Database::ensureConnected();
    QString sql = QStringLiteral(
        "SELECT a.id, a.service_date, r.name AS role_name, r.icon AS role_icon, "
        "a.member_id, m.name AS member_name, a.support_member_id, s.name AS support_name, a.notes, "
        "(SELECT n.status FROM non_availability_requests n "
        " WHERE n.assignment_id = a.id AND n.user_id = a.member_id "
        " ORDER BY n.created_at DESC LIMIT 1) AS request_status, "
        "EXISTS (SELECT 1 FROM availability_marks am "
        " WHERE am.user_id = a.member_id AND am.date = a.service_date) AS member_unavailable "
        "FROM assignments a "
        "JOIN assignment_roles r ON r.id = a.role_id "
        "LEFT JOIN users m ON m.id = a.member_id "
        "LEFT JOIN users s ON s.id = a.support_member_id "
        "WHERE a.service_date BETWEEN :from_date AND :to_date ");
    if (memberId > 0) {
        sql += QStringLiteral("AND (a.member_id = :member_id OR a.support_member_id = :member_id) ");
    }
    sql += QStringLiteral("ORDER BY a.service_date DESC, LOWER(r.name), a.id");

    QSqlQuery query;
    query.prepare(sql);
    query.bindValue(QStringLiteral(":from_date"), fromDate);
    query.bindValue(QStringLiteral(":to_date"), toDate);
    if (memberId > 0) {
        query.bindValue(QStringLiteral(":member_id"), memberId);
    }
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return rows;
    }
    while (query.next()) {
        ScheduleReportRow row;
        row.assignmentId = query.value(QStringLiteral("id")).toInt();
        row.serviceDate = query.value(QStringLiteral("service_date")).toDate();
        row.roleName = query.value(QStringLiteral("role_name")).toString();
        row.roleIcon = query.value(QStringLiteral("role_icon")).toString();
        const QVariant member = query.value(QStringLiteral("member_id"));
        row.memberId = member.isNull() ? -1 : member.toInt();
        row.memberName = query.value(QStringLiteral("member_name")).toString();
        const QVariant support = query.value(QStringLiteral("support_member_id"));
        row.supportMemberId = support.isNull() ? -1 : support.toInt();
        row.supportMemberName = query.value(QStringLiteral("support_name")).toString();
        row.notes = query.value(QStringLiteral("notes")).toString();
        row.requestStatus = query.value(QStringLiteral("request_status")).toString();
        row.memberMarkedUnavailable = query.value(QStringLiteral("member_unavailable")).toBool();
        rows.append(row);
    }
    return rows;
}

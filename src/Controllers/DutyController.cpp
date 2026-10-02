#include "DutyController.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "Database/Database.h"

namespace
{
    const QString kSelectColumns = QStringLiteral(
        "id, duty_type_id, service_date, member_id, support_member_id, notes");
}

DutyController::DutyController(QObject *parent)
    : QObject(parent)
{
}

static Duty dutyFromQuery(const QSqlQuery &query)
{
    Duty duty;
    duty.setId(query.value(QStringLiteral("id")).toInt());
    duty.setDutyTypeId(query.value(QStringLiteral("duty_type_id")).toInt());
    duty.setServiceDate(query.value(QStringLiteral("service_date")).toDate());
    const QVariant memberId = query.value(QStringLiteral("member_id"));
    duty.setMemberId(memberId.isNull() ? -1 : memberId.toInt());
    const QVariant supportMemberId = query.value(QStringLiteral("support_member_id"));
    duty.setSupportMemberId(supportMemberId.isNull() ? -1 : supportMemberId.toInt());
    duty.setNotes(query.value(QStringLiteral("notes")).toString());
    return duty;
}

QVector<Duty> DutyController::allDuties() const
{
    QVector<Duty> duties;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT %1 FROM duties ORDER BY service_date").arg(kSelectColumns));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return duties;
    }
    while (query.next()) {
        duties.append(dutyFromQuery(query));
    }
    return duties;
}

Duty DutyController::dutyById(int id) const
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT %1 FROM duties WHERE id = :id").arg(kSelectColumns));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return Duty();
    }
    return dutyFromQuery(query);
}

QVector<Duty> DutyController::dutiesForMember(int userId) const
{
    QVector<Duty> duties;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT %1 FROM duties "
        "WHERE (member_id = :user_id OR support_member_id = :user_id) AND service_date >= CURRENT_DATE "
        "ORDER BY service_date").arg(kSelectColumns));
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return duties;
    }
    while (query.next()) {
        duties.append(dutyFromQuery(query));
    }
    return duties;
}

QVector<Duty> DutyController::allDutiesForMember(int userId) const
{
    QVector<Duty> duties;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT %1 FROM duties "
        "WHERE member_id = :user_id OR support_member_id = :user_id "
        "ORDER BY service_date DESC").arg(kSelectColumns));
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return duties;
    }
    while (query.next()) {
        duties.append(dutyFromQuery(query));
    }
    return duties;
}

QVector<Duty> DutyController::dutiesForDate(const QDate &date) const
{
    QVector<Duty> duties;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT %1 FROM duties WHERE service_date = :service_date ORDER BY duty_type_id").arg(kSelectColumns));
    query.bindValue(QStringLiteral(":service_date"), date);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return duties;
    }
    while (query.next()) {
        duties.append(dutyFromQuery(query));
    }
    return duties;
}

bool DutyController::isEditableDate(const QDate &date)
{
    return date.isValid() && date >= QDate::currentDate();
}

namespace
{
    const QString kPastScheduleError = QStringLiteral("This Sunday has passed, so its schedule can't be changed.");
}

bool DutyController::addDuty(Duty &duty)
{
    if (!isEditableDate(duty.serviceDate())) {
        m_lastError = kPastScheduleError;
        return false;
    }
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT INTO duties (duty_type_id, service_date, member_id, support_member_id, notes) "
        "VALUES (:duty_type_id, :service_date, :member_id, :support_member_id, :notes) RETURNING id"));
    query.bindValue(QStringLiteral(":duty_type_id"), duty.dutyTypeId());
    query.bindValue(QStringLiteral(":service_date"), duty.serviceDate());
    query.bindValue(QStringLiteral(":member_id"),
        duty.memberId() > 0 ? QVariant(duty.memberId()) : QVariant());
    query.bindValue(QStringLiteral(":support_member_id"),
        duty.supportMemberId() > 0 ? QVariant(duty.supportMemberId()) : QVariant());
    query.bindValue(QStringLiteral(":notes"), duty.notes());
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return false;
    }
    duty.setId(query.value(0).toInt());
    emit dutiesChanged();
    return true;
}

bool DutyController::updateDuty(const Duty &duty)
{
    // Neither the duty's current Sunday nor the one it'd move to may be
    // in the past.
    if (!isEditableDate(duty.serviceDate()) || !isEditableDate(dutyById(duty.id()).serviceDate())) {
        m_lastError = kPastScheduleError;
        return false;
    }
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "UPDATE duties SET duty_type_id = :duty_type_id, service_date = :service_date, member_id = :member_id, "
        "support_member_id = :support_member_id, notes = :notes, updated_at = now() WHERE id = :id"));
    query.bindValue(QStringLiteral(":duty_type_id"), duty.dutyTypeId());
    query.bindValue(QStringLiteral(":service_date"), duty.serviceDate());
    query.bindValue(QStringLiteral(":member_id"),
        duty.memberId() > 0 ? QVariant(duty.memberId()) : QVariant());
    query.bindValue(QStringLiteral(":support_member_id"),
        duty.supportMemberId() > 0 ? QVariant(duty.supportMemberId()) : QVariant());
    query.bindValue(QStringLiteral(":notes"), duty.notes());
    query.bindValue(QStringLiteral(":id"), duty.id());
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    emit dutiesChanged();
    return true;
}

bool DutyController::removeDuty(int id)
{
    if (!isEditableDate(dutyById(id).serviceDate())) {
        m_lastError = kPastScheduleError;
        return false;
    }
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM duties WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    emit dutiesChanged();
    return true;
}

QStringList DutyController::membersMarkedUnavailable(const QDate &date) const
{
    QStringList names;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT DISTINCT u.name FROM duties a "
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

bool DutyController::copySchedule(
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
    // Copying *from* a past Sunday is fine; pasting onto one isn't.
    if (!isEditableDate(toDate)) {
        m_lastError = kPastScheduleError;
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
    countSource.prepare(QStringLiteral("SELECT COUNT(*) FROM duties WHERE service_date = :from_date"));
    countSource.bindValue(QStringLiteral(":from_date"), fromDate);
    if (!countSource.exec() || !countSource.next()) {
        return fail(countSource);
    }
    const int sourceCount = countSource.value(0).toInt();

    if (replaceExisting) {
        QSqlQuery clear;
        clear.prepare(QStringLiteral("DELETE FROM duties WHERE service_date = :to_date"));
        clear.bindValue(QStringLiteral(":to_date"), toDate);
        if (!clear.exec()) {
            return fail(clear);
        }
    }

    // IS NOT DISTINCT FROM so an unassigned (NULL member) slot for a duty type
    // also counts as "already there" and isn't duplicated.
    QSqlQuery insert;
    insert.prepare(QStringLiteral(
        "INSERT INTO duties (duty_type_id, service_date, member_id, support_member_id, notes) "
        "SELECT src.duty_type_id, CAST(:to_date AS DATE), src.member_id, src.support_member_id, src.notes "
        "FROM duties src "
        "WHERE src.service_date = :from_date "
        "AND NOT EXISTS (SELECT 1 FROM duties dst WHERE dst.service_date = :to_date "
        "AND dst.duty_type_id = src.duty_type_id AND dst.member_id IS NOT DISTINCT FROM src.member_id) "
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
    emit dutiesChanged();
    return true;
}

QVector<ScheduleReportRow> DutyController::scheduleReport(const QDate &fromDate, const QDate &toDate,
                                                         const QVector<int> &memberIds) const
{
    QVector<ScheduleReportRow> rows;
    Database::ensureConnected();
    QString sql = QStringLiteral(
        "SELECT a.id, a.service_date, r.name AS duty_type_name, r.icon AS duty_type_icon, "
        "a.member_id, m.name AS member_name, a.support_member_id, s.name AS support_name, a.notes, "
        "(SELECT n.status FROM non_availability_requests n "
        " WHERE n.duty_id = a.id AND n.user_id = a.member_id "
        " ORDER BY n.created_at DESC LIMIT 1) AS request_status, "
        "EXISTS (SELECT 1 FROM availability_marks am "
        " WHERE am.user_id = a.member_id AND am.date = a.service_date) AS member_unavailable "
        "FROM duties a "
        "JOIN duty_types r ON r.id = a.duty_type_id "
        "LEFT JOIN users m ON m.id = a.member_id "
        "LEFT JOIN users s ON s.id = a.support_member_id "
        "WHERE a.service_date BETWEEN :from_date AND :to_date ");
    if (!memberIds.isEmpty()) {
        // The ids are ints, so they can go into the SQL directly.
        QStringList ids;
        for (int id : memberIds) {
            ids.append(QString::number(id));
        }
        const QString list = ids.join(QLatin1Char(','));
        sql += QStringLiteral("AND (a.member_id IN (%1) OR a.support_member_id IN (%1)) ").arg(list);
    }
    sql += QStringLiteral("ORDER BY a.service_date DESC, LOWER(r.name), a.id");

    QSqlQuery query;
    query.prepare(sql);
    query.bindValue(QStringLiteral(":from_date"), fromDate);
    query.bindValue(QStringLiteral(":to_date"), toDate);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return rows;
    }
    while (query.next()) {
        ScheduleReportRow row;
        row.dutyId = query.value(QStringLiteral("id")).toInt();
        row.serviceDate = query.value(QStringLiteral("service_date")).toDate();
        row.dutyTypeName = query.value(QStringLiteral("duty_type_name")).toString();
        row.dutyTypeIcon = query.value(QStringLiteral("duty_type_icon")).toString();
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

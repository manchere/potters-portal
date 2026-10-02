#include "AvailabilityController.h"

#include <QSqlError>
#include <QSqlQuery>

#include "Database/Database.h"

AvailabilityController::AvailabilityController(QObject *parent)
    : QObject(parent)
{
}

QVector<AvailabilityMark> AvailabilityController::listForUser(int userId) const
{
    QVector<AvailabilityMark> marks;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT id, user_id, date FROM availability_marks WHERE user_id = :user_id ORDER BY date"));
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return marks;
    }
    while (query.next()) {
        marks.append(AvailabilityMark(
            query.value(0).toInt(), query.value(1).toInt(), query.value(2).toDate()));
    }
    return marks;
}

bool AvailabilityController::isMarked(int userId, const QDate &date) const
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT 1 FROM availability_marks WHERE user_id = :user_id AND date = :date"));
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":date"), date);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return query.next();
}

bool AvailabilityController::markUnavailable(int userId, const QDate &date)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT INTO availability_marks (user_id, date) VALUES (:user_id, :date) "
        "ON CONFLICT (user_id, date) DO NOTHING"));
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":date"), date);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

bool AvailabilityController::unmark(int userId, const QDate &date)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "DELETE FROM availability_marks WHERE user_id = :user_id AND date = :date"));
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":date"), date);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

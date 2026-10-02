#include "FeedbackController.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "Database/Database.h"

FeedbackController::FeedbackController(QObject *parent)
    : QObject(parent)
{
}

QVector<Feedback> FeedbackController::allFeedback() const
{
    QVector<Feedback> result;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT id, kind, member_id, subject, details, status, created_at FROM feedback "
        "ORDER BY (status = 'done'), created_at DESC"));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return result;
    }
    while (query.next()) {
        Feedback feedback;
        feedback.setId(query.value(0).toInt());
        feedback.setKind(Feedback::kindFromKey(query.value(1).toString()));
        feedback.setMemberId(query.value(2).isNull() ? -1 : query.value(2).toInt());
        feedback.setSubject(query.value(3).toString());
        feedback.setDetails(query.value(4).toString());
        feedback.setDone(query.value(5).toString() == QLatin1String("done"));
        feedback.setCreatedAt(query.value(6).toDateTime());
        result.append(feedback);
    }
    return result;
}

bool FeedbackController::addFeedback(Feedback &feedback)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT INTO feedback (kind, member_id, subject, details) "
        "VALUES (:kind, :member_id, :subject, :details) RETURNING id, created_at"));
    query.bindValue(QStringLiteral(":kind"), Feedback::kindKey(feedback.kind()));
    query.bindValue(QStringLiteral(":member_id"),
        feedback.memberId() > 0 ? QVariant(feedback.memberId()) : QVariant(QMetaType(QMetaType::LongLong)));
    query.bindValue(QStringLiteral(":subject"), feedback.subject());
    query.bindValue(QStringLiteral(":details"), feedback.details());
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return false;
    }
    feedback.setId(query.value(0).toInt());
    feedback.setCreatedAt(query.value(1).toDateTime());
    emit feedbackChanged();
    return true;
}

bool FeedbackController::setDone(int id, bool done)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("UPDATE feedback SET status = :status, updated_at = now() WHERE id = :id"));
    query.bindValue(QStringLiteral(":status"), done ? QStringLiteral("done") : QStringLiteral("open"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    emit feedbackChanged();
    return true;
}

bool FeedbackController::removeFeedback(int id)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM feedback WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    emit feedbackChanged();
    return true;
}

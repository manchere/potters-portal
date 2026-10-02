#pragma once

#include <QObject>
#include <QVector>

#include "Models/Feedback.h"

// Backed by Postgres (feedback table). See TeamController for the query
// pattern. Anyone can send a request; reading and handling them is an
// Admin job on the Feedback tab.
class FeedbackController : public QObject
{
    Q_OBJECT

public:
    explicit FeedbackController(QObject *parent = nullptr);

    // Open requests first, newest first within each.
    QVector<Feedback> allFeedback() const;

    QString lastError() const { return m_lastError; }

public slots:
    bool addFeedback(Feedback &feedback);
    bool setDone(int id, bool done);
    bool removeFeedback(int id);

signals:
    void feedbackChanged();

private:
    mutable QString m_lastError;
};

#pragma once

#include <QObject>
#include <QSet>
#include <QSqlDriver>
#include <QTimer>

// Tells this process when another app (the API server, another desktop)
// changes a table, so open views can refresh. Every change is recorded by
// database triggers in change_log and announced on the 'changes' channel
// (migration 0027):
//
//   * LISTEN on the app's connection hears about changes straight away;
//   * while polling is on, change_log is also read every 30 s, which
//     catches anything missed while the connection was down (Neon suspends
//     an idle compute and closes the socket) and covers pooled connection
//     strings, where LISTEN doesn't deliver anything.
//
// Changes this process made itself are skipped -- the controllers already
// emitted their own *Changed signals for those. Bursts (e.g. AI Fill adding
// a whole Sunday) come out as one tablesChanged.
class ChangeWatcher : public QObject
{
    Q_OBJECT

public:
    explicit ChangeWatcher(QObject *parent = nullptr);

    // Starts from the newest change so far (nothing before it is
    // reported), subscribes, and turns polling on.
    void start();

    // Polling keeps the database compute awake, so it's turned off while
    // nobody is looking (e.g. the window is minimized). Turning it back on
    // catches up straight away.
    void setPolling(bool on);

signals:
    // Table names, as in change_log.table_name (e.g. "duties", "songs").
    void tablesChanged(const QSet<QString> &tables);

private:
    void onNotification(const QString &name, QSqlDriver::NotificationSource source, const QVariant &payload);
    // Re-subscribes if the connection was re-opened since, then reports
    // change_log rows newer than the last one seen.
    void catchUp();
    void ensureSubscribed();
    void queue(const QString &table);

    bool m_started = false;
    // False when change_log doesn't exist (migration not applied); the
    // watcher then does nothing.
    bool m_available = true;
    qint64 m_lastId = 0;
    int m_ownPid = 0;
    QSet<QString> m_pending;
    QTimer m_pollTimer;
    QTimer m_flushTimer;
};

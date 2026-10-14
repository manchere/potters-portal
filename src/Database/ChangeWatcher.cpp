#include "ChangeWatcher.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtDebug>

#include <algorithm>
#include <utility>

#include "Database.h"

namespace
{
    const QString kChannel = QStringLiteral("changes");
    constexpr int kPollIntervalMs = 30 * 1000;
    // Long enough to gather one burst of trigger notifications.
    constexpr int kFlushDelayMs = 250;
}

ChangeWatcher::ChangeWatcher(QObject *parent)
    : QObject(parent)
{
    m_pollTimer.setInterval(kPollIntervalMs);
    connect(&m_pollTimer, &QTimer::timeout, this, &ChangeWatcher::catchUp);

    m_flushTimer.setSingleShot(true);
    m_flushTimer.setInterval(kFlushDelayMs);
    connect(&m_flushTimer, &QTimer::timeout, this, [this] {
        const QSet<QString> tables = std::exchange(m_pending, {});
        if (!tables.isEmpty()) {
            emit tablesChanged(tables);
        }
    });
}

void ChangeWatcher::start()
{
    if (m_started) {
        return;
    }
    m_started = true;

    Database::ensureConnected();
    QSqlQuery query;
    if (!query.exec(QStringLiteral("SELECT COALESCE(MAX(id), 0) FROM change_log")) || !query.next()) {
        qWarning() << "Live updates off -- change_log not readable (run database/migrate.py):"
                   << query.lastError().text();
        m_available = false;
        return;
    }
    m_lastId = query.value(0).toLongLong();

    ensureSubscribed();
    setPolling(true);
}

void ChangeWatcher::setPolling(bool on)
{
    if (!m_started || !m_available || on == m_pollTimer.isActive()) {
        return;
    }
    if (on) {
        catchUp();
        m_pollTimer.start();
    } else {
        m_pollTimer.stop();
    }
}

void ChangeWatcher::ensureSubscribed()
{
    QSqlDriver *driver = QSqlDatabase::database().driver();
    if (!driver || driver->subscribedToNotifications().contains(kChannel)) {
        return;
    }
    // A re-opened connection is a new server process: new pid, and the old
    // subscription went with the old one.
    QSqlQuery query;
    if (query.exec(QStringLiteral("SELECT pg_backend_pid()")) && query.next()) {
        m_ownPid = query.value(0).toInt();
    }
    // notification(name, source, payload) is emitted for every channel the
    // driver is subscribed to; connect once.
    disconnect(driver, nullptr, this, nullptr);
    connect(driver, &QSqlDriver::notification, this, &ChangeWatcher::onNotification);
    if (!driver->subscribeToNotification(kChannel)) {
        qWarning() << "Live updates: LISTEN failed, polling only:" << driver->lastError().text();
    }
}

void ChangeWatcher::onNotification(const QString &name, QSqlDriver::NotificationSource source,
                                   const QVariant &payload)
{
    if (name != kChannel) {
        return;
    }
    const QJsonObject change = QJsonDocument::fromJson(payload.toString().toUtf8()).object();
    m_lastId = std::max(m_lastId, change.value(QStringLiteral("id")).toInteger());
    if (source == QSqlDriver::SelfSource) {
        return;
    }
    queue(change.value(QStringLiteral("table")).toString());
}

void ChangeWatcher::catchUp()
{
    if (!m_available) {
        return;
    }
    Database::ensureConnected();
    ensureSubscribed();

    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT id, table_name, backend_pid FROM change_log WHERE id > :last ORDER BY id"));
    query.bindValue(QStringLiteral(":last"), m_lastId);
    if (!query.exec()) {
        return; // try again next tick
    }
    while (query.next()) {
        m_lastId = query.value(0).toLongLong();
        if (query.value(2).toInt() != m_ownPid) {
            queue(query.value(1).toString());
        }
    }
}

void ChangeWatcher::queue(const QString &table)
{
    if (table.isEmpty()) {
        return;
    }
    m_pending.insert(table);
    if (!m_flushTimer.isActive()) {
        m_flushTimer.start();
    }
}

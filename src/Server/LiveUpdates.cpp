#include "LiveUpdates.h"

#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerWebSocketUpgradeResponse>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QWebSocket>

#include <algorithm>

#include "Controllers/SessionController.h"

namespace
{
    const QString kPath = QStringLiteral("/api/live");
    constexpr int kAuthTimeoutMs = 10 * 1000;
    constexpr int kPingIntervalMs = 25 * 1000;
}

LiveUpdates::LiveUpdates(QHttpServer &server, SessionController &sessions, QObject *parent)
    : QObject(parent)
    , m_server(server)
    , m_sessions(sessions)
{
    m_server.addWebSocketUpgradeVerifier(this, [](const QHttpServerRequest &request) {
        return request.url().path() == kPath
            ? QHttpServerWebSocketUpgradeResponse::accept()
            : QHttpServerWebSocketUpgradeResponse::passToNext();
    });
    connect(&m_server, &QHttpServer::newWebSocketConnection, this, &LiveUpdates::acceptPending);

    m_watcher.setReportOwnChanges(true);
    connect(&m_watcher, &ChangeWatcher::tablesChanged, this, &LiveUpdates::broadcast);
    m_watcher.start();
    updatePolling();

    m_pingTimer.setInterval(kPingIntervalMs);
    connect(&m_pingTimer, &QTimer::timeout, this, [this] {
        QList<QWebSocket *> dead;
        for (auto it = m_clients.cbegin(); it != m_clients.cend(); ++it) {
            if (it.value().alive.hasExpired()) {
                dead.append(it.key());
            } else {
                it.key()->ping();
            }
        }
        // Not inside the loop: abort() can emit disconnected, which removes
        // the socket from m_clients.
        for (QWebSocket *socket : dead) {
            socket->abort();
        }
    });
    m_pingTimer.start();
}

void LiveUpdates::acceptPending()
{
    while (m_server.hasPendingWebSocketConnections()) {
        QWebSocket *socket = m_server.nextPendingWebSocketConnection().release();
        if (!socket) {
            continue;
        }
        socket->setParent(this);
        m_clients.insert(socket, Client{false, QDeadlineTimer(3 * kPingIntervalMs)});
        connect(socket, &QWebSocket::pong, this, [this, socket] {
            if (auto it = m_clients.find(socket); it != m_clients.end()) {
                it->alive.setRemainingTime(3 * kPingIntervalMs);
            }
        });
        connect(socket, &QWebSocket::textMessageReceived, this, [this, socket](const QString &message) {
            onMessage(socket, message);
        });
        connect(socket, &QWebSocket::disconnected, this, [this, socket] { onDisconnected(socket); });
        QTimer::singleShot(kAuthTimeoutMs, socket, [this, socket] {
            if (!m_clients.value(socket).signedIn) {
                socket->close(QWebSocketProtocol::CloseCodePolicyViolated, QStringLiteral("sign in first"));
            }
        });
    }
}

void LiveUpdates::onMessage(QWebSocket *socket, const QString &message)
{
    if (m_clients.value(socket).signedIn) {
        return; // signed in; clients have nothing else to say
    }
    const QJsonObject json = QJsonDocument::fromJson(message.toUtf8()).object();
    const QString token = json.value(QStringLiteral("token")).toString();
    if (json.value(QStringLiteral("type")).toString() != QStringLiteral("auth") || token.isEmpty()
        || m_sessions.userIdForToken(token) < 0) {
        socket->close(QWebSocketProtocol::CloseCodePolicyViolated, QStringLiteral("authentication required"));
        return;
    }
    m_clients[socket].signedIn = true;
    socket->sendTextMessage(QStringLiteral("{\"type\":\"ready\"}"));
    updatePolling();
}

void LiveUpdates::onDisconnected(QWebSocket *socket)
{
    m_clients.remove(socket);
    socket->deleteLater();
    updatePolling();
}

void LiveUpdates::broadcast(const QSet<QString> &tables)
{
    QStringList names(tables.begin(), tables.end());
    std::sort(names.begin(), names.end());
    const QString message = QString::fromUtf8(QJsonDocument(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("changes")},
        {QStringLiteral("tables"), QJsonArray::fromStringList(names)},
    }).toJson(QJsonDocument::Compact));
    for (auto it = m_clients.cbegin(); it != m_clients.cend(); ++it) {
        if (it.value().signedIn) {
            it.key()->sendTextMessage(message);
        }
    }
}

void LiveUpdates::updatePolling()
{
    m_watcher.setPolling(std::any_of(m_clients.cbegin(), m_clients.cend(),
                                     [](const Client &client) { return client.signedIn; }));
}

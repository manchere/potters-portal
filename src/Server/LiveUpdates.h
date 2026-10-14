#pragma once

#include <QDeadlineTimer>
#include <QHash>
#include <QObject>
#include <QTimer>

#include "Database/ChangeWatcher.h"

class QHttpServer;
class QWebSocket;
class SessionController;

// Live updates for the mobile / web app, over a WebSocket at /api/live.
//
// A client opens the socket and sends {"type": "auth", "token": "<session
// token>"} as its first message (browsers can't put an Authorization header
// on a WebSocket, and a token in the URL ends up in logs); a socket that
// hasn't signed in within 10 s is closed. From then on it receives
//
//   {"type": "changes", "tables": ["duties", "non_availability_requests"]}
//
// whenever anything -- this server, the desktop app -- changes those tables
// (see ChangeWatcher), and re-reads what it shows. Nothing is sent for what
// happened while a client was disconnected, so a client reloads after
// reconnecting.
//
// change_log is only polled while at least one client is connected, so an
// idle server doesn't keep the database awake.
class LiveUpdates : public QObject
{
    Q_OBJECT

public:
    LiveUpdates(QHttpServer &server, SessionController &sessions, QObject *parent = nullptr);

private:
    void acceptPending();
    void onMessage(QWebSocket *socket, const QString &message);
    void onDisconnected(QWebSocket *socket);
    void broadcast(const QSet<QString> &tables);
    void updatePolling();

    QHttpServer &m_server;
    SessionController &m_sessions;
    ChangeWatcher m_watcher;
    struct Client
    {
        bool signedIn = false;
        // Pushed back by every pong; a socket that stops answering (phone
        // went offline without closing it) is closed when it runs out.
        QDeadlineTimer alive;
    };
    // Every open socket (owned by this).
    QHash<QWebSocket *, Client> m_clients;
    // Proxies (Render's included) drop connections that stay silent; pings
    // also find dead ones.
    QTimer m_pingTimer;
};

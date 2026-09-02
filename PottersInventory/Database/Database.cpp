#include "Database.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>
#include <QUrlQuery>

namespace Database
{
    bool connect(const QString &uri, QString *errorMessage)
    {
        const QUrl url(uri);
        if (!url.isValid() || url.host().isEmpty() || url.path().length() < 2) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Invalid database URL: %1").arg(uri);
            }
            return false;
        }

        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"));
        db.setHostName(url.host());
        db.setPort(url.port(5432));
        db.setDatabaseName(url.path().mid(1)); // strip leading '/'
        db.setUserName(url.userName());
        db.setPassword(url.password());

        QStringList options;
        const QUrlQuery query(url);
        for (const auto &item : query.queryItems()) {
            options << QStringLiteral("%1=%2").arg(item.first, item.second);
        }
        db.setConnectOptions(options.join(QLatin1Char(';')));

        if (!db.open()) {
            if (errorMessage) {
                *errorMessage = db.lastError().text();
            }
            return false;
        }
        return true;
    }

    bool isConnected()
    {
        return QSqlDatabase::database().isOpen();
    }

    bool ensureConnected()
    {
        QSqlDatabase db = QSqlDatabase::database();
        if (db.isOpen()) {
            // isOpen() only reflects the driver's own state; it doesn't
            // detect a socket the server side already closed (e.g. Neon
            // suspending an idle compute), so ping before trusting it.
            QSqlQuery ping(db);
            if (ping.exec(QStringLiteral("SELECT 1"))) {
                return true;
            }
        }
        db.close();
        return db.open();
    }
}

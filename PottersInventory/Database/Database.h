#pragma once

#include <QString>

// Thin wrapper around the app's single QSqlDatabase connection (driver:
// QPSQL). Controllers issue QSqlQuery against QSqlDatabase::database()
// once this has connected successfully.
namespace Database
{
    // uri is a standard Postgres connection string, e.g.
    // "postgresql://user:password@host/dbname?sslmode=require" (this is
    // exactly what Neon gives you on the project's Connection Details page).
    bool connect(const QString &uri, QString *errorMessage);

    // Re-opens the connection if it has been dropped (e.g. Neon suspending
    // an idle compute). Cheap no-op when already open; call at the start of
    // every Controller method that's about to run a query, since a
    // long-lived server process can sit idle between requests.
    bool ensureConnected();
}

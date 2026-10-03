#pragma once

#include <QByteArray>
#include <QSettings>
#include <QString>

// The Groq API key for photo auto-fill and search questions: GROQ_API_KEY
// if set (developers, the Render server), otherwise the key saved on this
// computer from the database setup dialog (DatabaseSetupDialog, saved by
// Portal.cpp). Empty when neither is there.
inline QString savedGroqApiKeySetting() { return QStringLiteral("groqApiKey"); }

inline QByteArray groqApiKey()
{
    const QString envKey = qEnvironmentVariable("GROQ_API_KEY");
    if (!envKey.isEmpty()) {
        return envKey.toUtf8();
    }
    const QSettings settings(QStringLiteral("PottersPortal"), QStringLiteral("PottersPortal"));
    return settings.value(savedGroqApiKeySetting()).toString().trimmed().toUtf8();
}

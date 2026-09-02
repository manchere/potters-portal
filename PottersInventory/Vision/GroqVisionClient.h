#pragma once

#include <QJsonObject>
#include <QString>

class QNetworkAccessManager;

// Wraps the Groq vision API call used to suggest an item's name/description
// from a photo. Shared by the REST server (Server/ServerMain.cpp) and the
// desktop app (Views/ItemFormWidget.cpp) so both "Add Item" flows go through
// identical prompt/model/parsing logic.
namespace GroqVision
{
    // imageDataUrl is a data URL ("data:image/jpeg;base64,...").
    // Returns {"name": "...", "description": "..."} on success, or an empty
    // object with *errorMessage set on failure (missing API key, network
    // error, or a response that couldn't be parsed as the expected JSON).
    QJsonObject describeItem(QNetworkAccessManager &manager, const QString &imageDataUrl, QString *errorMessage);
}

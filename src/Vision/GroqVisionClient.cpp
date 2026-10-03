#include "GroqVisionClient.h"
#include "GroqApiKey.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace
{
    // QNetworkAccessManager is signal/async by nature; both callers of this
    // (a synchronous HTTP route handler, and a synchronous button-click
    // handler) want a simple blocking call rather than reworking around
    // QFuture for this one request, so this blocks on a local event loop.
    QByteArray postJsonSync(QNetworkAccessManager &manager, const QUrl &url, const QByteArray &authHeader,
                             const QByteArray &body, int *statusCode)
    {
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
        request.setRawHeader("Authorization", authHeader);

        QNetworkReply *reply = manager.post(request, body);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer::singleShot(30000, &loop, &QEventLoop::quit);
        loop.exec();

        *statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray data = reply->readAll();
        reply->deleteLater();
        return data;
    }
}

namespace GroqVision
{
    QJsonObject describeItem(QNetworkAccessManager &manager, const QString &imageDataUrl, QString *errorMessage)
    {
        const QByteArray apiKey = groqApiKey();
        if (apiKey.isEmpty()) {
            *errorMessage = QStringLiteral("No Groq API key: set GROQ_API_KEY or enter one when connecting to the database.");
            return {};
        }
        // Not every account has the same models enabled; verified via
        // GET https://api.groq.com/openai/v1/models that this is the only one
        // on this account whose input_modalities include "image".
        const QString model = qEnvironmentVariable("GROQ_VISION_MODEL", QStringLiteral("qwen/qwen3.6-27b"));

        const QJsonArray content{
            QJsonObject{
                {QStringLiteral("type"), QStringLiteral("text")},
                {QStringLiteral("text"), QStringLiteral(
                    "You are helping catalog items for a church inventory system. Look at this "
                    "photo and reply with ONLY a raw JSON object (no markdown fences, no other "
                    "text) with exactly two string fields: \"name\" (a short 2-5 word item name) "
                    "and \"description\" (one concise sentence describing the item and its "
                    "visible condition).")}
            },
            QJsonObject{
                {QStringLiteral("type"), QStringLiteral("image_url")},
                {QStringLiteral("image_url"), QJsonObject{{QStringLiteral("url"), imageDataUrl}}}
            }
        };
        const QJsonObject payload{
            {QStringLiteral("model"), model},
            {QStringLiteral("messages"), QJsonArray{QJsonObject{
                {QStringLiteral("role"), QStringLiteral("user")},
                {QStringLiteral("content"), content}
            }}},
            {QStringLiteral("temperature"), 0.2},
            // qwen3.6 is a reasoning model that spends tokens on a <think>...</think>
            // block before the actual answer, so this needs real headroom, not
            // just enough for the final JSON.
            {QStringLiteral("max_tokens"), 1024},
            {QStringLiteral("reasoning_effort"), QStringLiteral("none")},
            {QStringLiteral("response_format"), QJsonObject{{QStringLiteral("type"), QStringLiteral("json_object")}}}
        };

        int statusCode = 0;
        const QByteArray responseBytes = postJsonSync(
            manager, QUrl(QStringLiteral("https://api.groq.com/openai/v1/chat/completions")),
            "Bearer " + apiKey, QJsonDocument(payload).toJson(QJsonDocument::Compact), &statusCode);

        if (statusCode < 200 || statusCode >= 300) {
            *errorMessage = QStringLiteral("Groq API error (%1): %2").arg(statusCode).arg(QString::fromUtf8(responseBytes));
            return {};
        }

        const QJsonObject responseObject = QJsonDocument::fromJson(responseBytes).object();
        QString content_ = responseObject[QStringLiteral("choices")].toArray().first()
            .toObject()[QStringLiteral("message")].toObject()[QStringLiteral("content")].toString().trimmed();

        // Reasoning models emit a <think>...</think> block before the real
        // answer; only the part after it is the actual response.
        const int thinkEnd = content_.lastIndexOf(QLatin1String("</think>"));
        if (thinkEnd >= 0) {
            content_ = content_.mid(thinkEnd + 8).trimmed();
        }

        // Models sometimes wrap the JSON in a ```json ... ``` fence despite being
        // told not to; strip that if present before parsing.
        if (content_.startsWith(QLatin1String("```"))) {
            const int firstNewline = content_.indexOf(QLatin1Char('\n'));
            const int lastFence = content_.lastIndexOf(QLatin1String("```"));
            if (firstNewline >= 0 && lastFence > firstNewline) {
                content_ = content_.mid(firstNewline + 1, lastFence - firstNewline - 1).trimmed();
            }
        }

        const QJsonObject suggestion = QJsonDocument::fromJson(content_.toUtf8()).object();
        if (suggestion.isEmpty()) {
            *errorMessage = QStringLiteral("Could not parse a suggestion from the model's response: %1").arg(content_);
            return {};
        }
        return suggestion;
    }
}

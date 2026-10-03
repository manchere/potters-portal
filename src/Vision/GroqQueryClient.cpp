#include "GroqQueryClient.h"
#include "GroqApiKey.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace {

// Same blocking-HTTP-post shape as GroqVisionClient.cpp's postJsonSync;
// kept as its own small copy here rather than shared since it's a dozen
// lines and the two clients otherwise have nothing in common (this one
// is text-only, no image payload).
QByteArray postJsonSync(QNetworkAccessManager &manager, const QUrl &url, const QByteArray &authHeader,
                         const QByteArray &body, int *statusCode)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", authHeader);

    QNetworkReply *reply = manager.post(request, body);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(15000, &loop, &QEventLoop::quit);
    loop.exec();

    *statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray data = reply->readAll();
    reply->deleteLater();
    return data;
}

QJsonArray toJsonArray(const QStringList &values)
{
    QJsonArray array;
    for (const QString &value : values) {
        array.append(value);
    }
    return array;
}

QStringList toStringList(const QJsonValue &value)
{
    QStringList result;
    for (const QJsonValue &entry : value.toArray()) {
        const QString text = entry.toString();
        if (!text.isEmpty()) {
            result << text;
        }
    }
    return result;
}

} // namespace

namespace GroqQuery
{
    ItemFilter interpretQuestion(QNetworkAccessManager &manager, const QString &question,
                                  const QStringList &knownStatuses, const QStringList &knownCategories,
                                  const QStringList &knownTags, QString *errorMessage)
    {
        const QByteArray apiKey = groqApiKey();
        if (apiKey.isEmpty()) {
            *errorMessage = QStringLiteral("No Groq API key: set GROQ_API_KEY or enter one when connecting to the database.");
            return {};
        }
        const QString model = qEnvironmentVariable("GROQ_QUERY_MODEL", QStringLiteral("llama-3.1-8b-instant"));

        const QJsonObject instructions{
            {QStringLiteral("known_statuses"), toJsonArray(knownStatuses)},
            {QStringLiteral("known_categories"), toJsonArray(knownCategories)},
            {QStringLiteral("known_tags"), toJsonArray(knownTags)},
        };
        const QString systemPrompt = QStringLiteral(
            "You turn a question about a physical inventory into a JSON search filter. "
            "Reply with ONLY a raw JSON object (no markdown fences, no other text) with "
            "exactly four fields: \"status\" (one value from known_statuses, or empty "
            "string if the question doesn't mention a status), \"category\" (one value "
            "from known_categories, or empty string), \"tags\" (an array of zero or more "
            "values from known_tags), and \"keywords\" (any remaining significant words "
            "from the question useful for a free-text name/description search, or empty "
            "string). Only use values that literally appear in the known_* lists provided "
            "— never invent one. Context: %1")
            .arg(QString::fromUtf8(QJsonDocument(instructions).toJson(QJsonDocument::Compact)));

        const QJsonObject payload{
            {QStringLiteral("model"), model},
            {QStringLiteral("messages"), QJsonArray{
                QJsonObject{{QStringLiteral("role"), QStringLiteral("system")}, {QStringLiteral("content"), systemPrompt}},
                QJsonObject{{QStringLiteral("role"), QStringLiteral("user")}, {QStringLiteral("content"), question}}
            }},
            {QStringLiteral("temperature"), 0.1},
            {QStringLiteral("max_tokens"), 512},
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
        QString content = responseObject[QStringLiteral("choices")].toArray().first()
            .toObject()[QStringLiteral("message")].toObject()[QStringLiteral("content")].toString().trimmed();

        // Defensive: strip a reasoning block or markdown fence if the
        // configured model emits one, same as GroqVisionClient.cpp.
        const int thinkEnd = content.lastIndexOf(QLatin1String("</think>"));
        if (thinkEnd >= 0) {
            content = content.mid(thinkEnd + 8).trimmed();
        }
        if (content.startsWith(QLatin1String("```"))) {
            const int firstNewline = content.indexOf(QLatin1Char('\n'));
            const int lastFence = content.lastIndexOf(QLatin1String("```"));
            if (firstNewline >= 0 && lastFence > firstNewline) {
                content = content.mid(firstNewline + 1, lastFence - firstNewline - 1).trimmed();
            }
        }

        const QJsonObject parsed = QJsonDocument::fromJson(content.toUtf8()).object();
        if (parsed.isEmpty()) {
            *errorMessage = QStringLiteral("Could not parse a filter from the model's response: %1").arg(content);
            return {};
        }

        ItemFilter filter;
        filter.status = parsed.value(QStringLiteral("status")).toString();
        filter.category = parsed.value(QStringLiteral("category")).toString();
        filter.tags = toStringList(parsed.value(QStringLiteral("tags")));
        filter.keywords = parsed.value(QStringLiteral("keywords")).toString();
        return filter;
    }
}

#include "AiScheduleFiller.h"

#include <algorithm>

#include <QEventLoop>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <QTimer>
#include <QUrl>

#include "Controllers/AvailabilityController.h"
#include "Controllers/DutyController.h"
#include "Controllers/DutyTypeController.h"
#include "Controllers/UserController.h"
#include "Vision/GroqApiKey.h"

namespace {

// How far back the model looks to learn who usually does what.
constexpr int kHistoryMonths = 6;

// Same blocking post as GroqQueryClient.cpp, with a longer wait: a whole
// month of Sundays is a much bigger answer than a search filter.
QByteArray postJsonSync(QNetworkAccessManager &manager, const QUrl &url, const QByteArray &authHeader,
                         const QByteArray &body, int *statusCode)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", authHeader);

    QNetworkReply *reply = manager.post(request, body);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(60000, &loop, &QEventLoop::quit);
    loop.exec();

    *statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray data = reply->readAll();
    reply->deleteLater();
    return data;
}

// Strips a reasoning block or markdown fence, as GroqQueryClient.cpp does.
QString cleanContent(QString content)
{
    content = content.trimmed();
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
    return content;
}

QJsonArray toJsonArray(const QSet<int> &ids)
{
    QList<int> sorted = ids.values();
    std::sort(sorted.begin(), sorted.end());
    QJsonArray array;
    for (int id : sorted) {
        array.append(id);
    }
    return array;
}

// A null, missing or non-numeric id comes out as -1.
int idValue(const QJsonValue &value)
{
    return value.isDouble() ? value.toInt(-1) : -1;
}

} // namespace

namespace AiSchedule
{
    QVector<QDate> upcomingSundays(int count)
    {
        QVector<QDate> sundays;
        const QDate today = QDate::currentDate();
        // QDate::dayOfWeek(): 1 = Monday ... 7 = Sunday.
        QDate sunday = today.addDays(7 - today.dayOfWeek());
        for (int i = 0; i < count; ++i, sunday = sunday.addDays(7)) {
            sundays.append(sunday);
        }
        return sundays;
    }

    Result propose(QNetworkAccessManager &manager,
                   const DutyController &duties,
                   const DutyTypeController &dutyTypes,
                   const UserController &users,
                   const AvailabilityController &availability,
                   const QVector<QDate> &sundays,
                   const QVector<int> &dutyTypeIds,
                   const QString &instructions)
    {
        Result result;
        const QByteArray apiKey = groqApiKey();
        if (apiKey.isEmpty()) {
            result.error = QStringLiteral("No Groq API key: set GROQ_API_KEY or enter one when connecting to the database.");
            return result;
        }

        // Only Sundays that can still be changed.
        QVector<QDate> targetSundays;
        for (const QDate &date : sundays) {
            if (date.isValid() && date.dayOfWeek() == Qt::Sunday && DutyController::isEditableDate(date)
                && !targetSundays.contains(date)) {
                targetSundays.append(date);
            }
        }
        std::sort(targetSundays.begin(), targetSundays.end());
        if (targetSundays.isEmpty()) {
            result.error = QStringLiteral("Pick at least one upcoming Sunday.");
            return result;
        }

        QHash<int, QString> dutyTypeNames;
        QJsonArray dutyTypesJson;
        for (const DutyType &dutyType : dutyTypes.allDutyTypes()) {
            if (!dutyTypeIds.isEmpty() && !dutyTypeIds.contains(dutyType.id())) {
                continue;
            }
            dutyTypeNames.insert(dutyType.id(), dutyType.name());
            dutyTypesJson.append(QJsonObject{
                {QStringLiteral("id"), dutyType.id()},
                {QStringLiteral("name"), dutyType.name()},
            });
        }
        if (dutyTypeNames.isEmpty()) {
            result.error = QStringLiteral("There are no duty types to fill.");
            return result;
        }

        const QVector<User> members = users.allUsers();
        if (members.isEmpty()) {
            result.error = QStringLiteral("There are no members to schedule.");
            return result;
        }
        QSet<int> memberIds;
        for (const User &member : members) {
            memberIds.insert(member.id());
        }

        // Each Sunday: who's on already, which chosen duty types are still
        // empty, and who marked it unavailable.
        const QDate historyStart = QDate::currentDate().addMonths(-kHistoryMonths);
        QHash<QDate, QSet<int>> serving;
        QHash<QDate, QSet<int>> filledTypes;
        // memberId -> dutyTypeId -> (times, last date), from the history window.
        QHash<int, QHash<int, QPair<int, QDate>>> history;
        QHash<int, int> alreadyOnTargets;
        for (const Duty &duty : duties.allDuties()) {
            const QDate date = duty.serviceDate();
            if (targetSundays.contains(date)) {
                filledTypes[date].insert(duty.dutyTypeId());
                for (int id : {duty.memberId(), duty.supportMemberId()}) {
                    if (id > 0) {
                        serving[date].insert(id);
                    }
                }
                if (duty.memberId() > 0) {
                    ++alreadyOnTargets[duty.memberId()];
                }
            }
            if (date >= historyStart && duty.memberId() > 0) {
                QPair<int, QDate> &entry = history[duty.memberId()][duty.dutyTypeId()];
                ++entry.first;
                entry.second = std::max(entry.second, date);
            }
        }
        QHash<QDate, QSet<int>> away;
        for (const AvailabilityMark &mark : availability.marksBetween(targetSundays.first(), targetSundays.last())) {
            away[mark.date()].insert(mark.userId());
        }

        QHash<QDate, QSet<int>> gaps;
        QJsonArray sundaysJson;
        for (const QDate &date : targetSundays) {
            QSet<int> open;
            for (auto it = dutyTypeNames.cbegin(); it != dutyTypeNames.cend(); ++it) {
                if (!filledTypes.value(date).contains(it.key())) {
                    open.insert(it.key());
                }
            }
            if (open.isEmpty()) {
                continue;
            }
            gaps.insert(date, open);
            sundaysJson.append(QJsonObject{
                {QStringLiteral("date"), date.toString(Qt::ISODate)},
                {QStringLiteral("duty_type_ids_to_fill"), toJsonArray(open)},
                {QStringLiteral("already_serving_member_ids"), toJsonArray(serving.value(date))},
                {QStringLiteral("unavailable_member_ids"), toJsonArray(away.value(date))},
            });
        }
        if (gaps.isEmpty()) {
            return result; // every chosen duty is already filled
        }

        QJsonArray membersJson;
        for (const User &member : members) {
            QJsonArray past;
            const QHash<int, QPair<int, QDate>> memberHistory = history.value(member.id());
            for (auto it = memberHistory.cbegin(); it != memberHistory.cend(); ++it) {
                past.append(QJsonObject{
                    {QStringLiteral("duty_type_id"), it.key()},
                    {QStringLiteral("times"), it.value().first},
                    {QStringLiteral("last_date"), it.value().second.toString(Qt::ISODate)},
                });
            }
            membersJson.append(QJsonObject{
                {QStringLiteral("id"), member.id()},
                {QStringLiteral("name"), member.name()},
                {QStringLiteral("recent_duties"), past},
                {QStringLiteral("duties_already_on_these_sundays"), alreadyOnTargets.value(member.id())},
            });
        }

        const QJsonObject context{
            {QStringLiteral("duty_types"), dutyTypesJson},
            {QStringLiteral("members"), membersJson},
            {QStringLiteral("sundays"), sundaysJson},
        };
        const QString systemPrompt = QStringLiteral(
            "You draw up a church's Sunday serving rota. For every Sunday in \"sundays\", "
            "assign one member to each id in its duty_type_ids_to_fill, plus an optional "
            "backup member who covers if they're absent.\n"
            "Rules, most important first:\n"
            "1. Never assign anyone (as member or backup) who is in that Sunday's "
            "unavailable_member_ids.\n"
            "2. Use only ids from duty_types and members. Never invent one.\n"
            "3. Prefer members whose recent_duties include that duty type -- they know how "
            "to do it. Only give a duty to someone new to it when nobody experienced is free.\n"
            "4. Share the work fairly: rotate between the experienced members, favour whoever "
            "did it least recently, and count duties_already_on_these_sundays.\n"
            "5. Avoid giving one person two duties on the same Sunday, and avoid people in "
            "already_serving_member_ids, unless there's nobody else.\n"
            "6. The backup must be a different person from the member, ideally someone who "
            "has done that duty before; use null when there's no good choice.\n"
            "7. Follow the Admin's notes, if any, unless they break rule 1 or 2.\n"
            "Reply with ONLY a raw JSON object (no markdown, no other text): "
            "{\"assignments\": [{\"date\": \"YYYY-MM-DD\", \"duty_type_id\": 1, \"member_id\": 2, "
            "\"support_member_id\": 3 or null}]}. Leave a duty out if nobody can do it.\n"
            "Data: %1")
            .arg(QString::fromUtf8(QJsonDocument(context).toJson(QJsonDocument::Compact)));
        const QString userPrompt = instructions.trimmed().isEmpty()
            ? QStringLiteral("Fill the rota.")
            : QStringLiteral("Fill the rota. Admin's notes: %1").arg(instructions.trimmed());

        const QString model = qEnvironmentVariable("GROQ_SCHEDULE_MODEL", QStringLiteral("llama-3.3-70b-versatile"));
        const QJsonObject payload{
            {QStringLiteral("model"), model},
            {QStringLiteral("messages"), QJsonArray{
                QJsonObject{{QStringLiteral("role"), QStringLiteral("system")}, {QStringLiteral("content"), systemPrompt}},
                QJsonObject{{QStringLiteral("role"), QStringLiteral("user")}, {QStringLiteral("content"), userPrompt}}
            }},
            {QStringLiteral("temperature"), 0.3},
            {QStringLiteral("max_tokens"), 4096},
            {QStringLiteral("response_format"), QJsonObject{{QStringLiteral("type"), QStringLiteral("json_object")}}}
        };

        int statusCode = 0;
        const QByteArray responseBytes = postJsonSync(
            manager, QUrl(QStringLiteral("https://api.groq.com/openai/v1/chat/completions")),
            "Bearer " + apiKey, QJsonDocument(payload).toJson(QJsonDocument::Compact), &statusCode);
        if (statusCode == 0) {
            result.error = QStringLiteral("Couldn't reach the AI service -- check the internet connection and try again.");
            return result;
        }
        if (statusCode < 200 || statusCode >= 300) {
            result.error = QStringLiteral("Groq API error (%1): %2").arg(statusCode).arg(QString::fromUtf8(responseBytes));
            return result;
        }

        const QJsonArray choices = QJsonDocument::fromJson(responseBytes).object()
            .value(QStringLiteral("choices")).toArray();
        const QString content = cleanContent(choices.isEmpty() ? QString()
            : choices.first().toObject()[QStringLiteral("message")].toObject()[QStringLiteral("content")].toString());
        const QJsonObject parsed = QJsonDocument::fromJson(content.toUtf8()).object();
        if (!parsed.value(QStringLiteral("assignments")).isArray()) {
            result.error = QStringLiteral("The AI's answer couldn't be read: %1").arg(content.left(300));
            return result;
        }

        // Keep the answers that obey the hard rules, first come first served.
        QHash<QDate, QSet<int>> open = gaps;
        for (const QJsonValue &value : parsed.value(QStringLiteral("assignments")).toArray()) {
            const QJsonObject entry = value.toObject();
            Proposal proposal;
            proposal.date = QDate::fromString(entry.value(QStringLiteral("date")).toString(), Qt::ISODate);
            proposal.dutyTypeId = idValue(entry.value(QStringLiteral("duty_type_id")));
            proposal.memberId = idValue(entry.value(QStringLiteral("member_id")));
            proposal.supportMemberId = idValue(entry.value(QStringLiteral("support_member_id")));
            if (!open.value(proposal.date).contains(proposal.dutyTypeId)
                || !memberIds.contains(proposal.memberId)
                || away.value(proposal.date).contains(proposal.memberId)) {
                continue;
            }
            // A bad backup only loses the backup, not the whole duty.
            if (!memberIds.contains(proposal.supportMemberId) || proposal.supportMemberId == proposal.memberId
                || away.value(proposal.date).contains(proposal.supportMemberId)) {
                proposal.supportMemberId = -1;
            }
            open[proposal.date].remove(proposal.dutyTypeId);
            result.proposals.append(proposal);
        }

        for (const QDate &date : targetSundays) {
            for (int dutyTypeId : open.value(date)) {
                result.unfilled.append(Slot{date, dutyTypeId});
            }
        }
        const auto byDateThenName = [&dutyTypeNames](const QDate &dateA, int typeA, const QDate &dateB, int typeB) {
            if (dateA != dateB) {
                return dateA < dateB;
            }
            return dutyTypeNames.value(typeA).localeAwareCompare(dutyTypeNames.value(typeB)) < 0;
        };
        std::sort(result.proposals.begin(), result.proposals.end(), [&](const Proposal &a, const Proposal &b) {
            return byDateThenName(a.date, a.dutyTypeId, b.date, b.dutyTypeId);
        });
        std::sort(result.unfilled.begin(), result.unfilled.end(), [&](const Slot &a, const Slot &b) {
            return byDateThenName(a.date, a.dutyTypeId, b.date, b.dutyTypeId);
        });
        return result;
    }
}

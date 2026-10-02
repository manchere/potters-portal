#pragma once

#include <QString>
#include <QStringList>

class QNetworkAccessManager;

// Wraps a text-only Groq call that turns a natural-language question about
// the inventory ("what's broken in the storage room?") into a structured
// filter the Items table can apply directly. Used only when the network is
// reachable; ItemListView falls back to a local keyword parser otherwise.
namespace GroqQuery
{
    struct ItemFilter
    {
        QString status;      // one of knownStatuses, or empty for "any"
        QString category;    // one of knownCategories, or empty for "any"
        QStringList tags;    // subset of knownTags
        QString keywords;    // free text to fuzzy-match against name/description/location
    };

    // knownStatuses/knownCategories/knownTags are the actual values
    // currently in the database, so the model can only pick real ones.
    // Returns a default-constructed (all-empty) ItemFilter with
    // *errorMessage set on failure (missing API key, network error, or an
    // unparsable response) — callers should treat that the same as "no
    // filter matched" and fall back to local parsing.
    ItemFilter interpretQuestion(QNetworkAccessManager &manager, const QString &question,
                                  const QStringList &knownStatuses, const QStringList &knownCategories,
                                  const QStringList &knownTags, QString *errorMessage);
}

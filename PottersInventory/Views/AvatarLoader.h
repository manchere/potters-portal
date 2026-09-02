#pragma once

#include <QString>

class QLabel;
class QNetworkAccessManager;

// Fetches a DiceBear avatar PNG for a given seed and sets it as a QLabel's
// pixmap once loaded, asynchronously -- unlike Vision/GroqVisionClient's
// blocking helper (fine for a single one-off AI call), this may be firing
// off many requests at once for a list of members and must not freeze the UI.
namespace AvatarLoader
{
    void loadInto(QNetworkAccessManager &manager, const QString &seed, QLabel *label, int sizePx = 40);
}

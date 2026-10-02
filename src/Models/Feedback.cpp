#include "Feedback.h"

QString Feedback::kindKey(Kind kind)
{
    switch (kind) {
    case Kind::Feature: return QStringLiteral("feature");
    case Kind::Profile: return QStringLiteral("profile");
    case Kind::Bug: break;
    }
    return QStringLiteral("bug");
}

Feedback::Kind Feedback::kindFromKey(const QString &key)
{
    if (key == QLatin1String("feature")) {
        return Kind::Feature;
    }
    if (key == QLatin1String("profile")) {
        return Kind::Profile;
    }
    return Kind::Bug;
}

#include "RoleDisplay.h"

namespace RoleDisplay
{
    QString icon(AssignmentRole role)
    {
        switch (role) {
        case AssignmentRole::Singing: return QStringLiteral("\U0001F3A4");        // microphone
        case AssignmentRole::Translating: return QStringLiteral("\U0001F310");    // globe with meridians
        case AssignmentRole::Preaching: return QStringLiteral("\U0001F4D6");      // open book
        case AssignmentRole::Offering: return QStringLiteral("\U0001F4B0");       // money bag
        case AssignmentRole::Announcement: return QStringLiteral("\U0001F4E2");   // loudspeaker
        }
        return QString();
    }

    QString label(AssignmentRole role)
    {
        switch (role) {
        case AssignmentRole::Singing: return QStringLiteral("Singing");
        case AssignmentRole::Translating: return QStringLiteral("Translating");
        case AssignmentRole::Preaching: return QStringLiteral("Preaching");
        case AssignmentRole::Offering: return QStringLiteral("Offering");
        case AssignmentRole::Announcement: return QStringLiteral("Announcement");
        }
        return QString();
    }

    QString iconAndLabel(AssignmentRole role)
    {
        return icon(role) + QStringLiteral("  ") + label(role);
    }
}

#pragma once

#include <QList>
#include <QPair>
#include <QString>

// A Member's profile circle color (shown with their initials wherever a
// Member appears). Any "#rrggbb" is allowed; palette() is just the quick
// picks offered first. Kept in step with mobile/src/api/memberColors.ts.
namespace MemberColors
{
    // (name, "#rrggbb") in display order.
    const QList<QPair<QString, QString>> &palette();

    QString defaultColor();

    // True when `color` is a "#rrggbb" hex color (any case).
    bool isValid(const QString &color);

    // White, or a near-black for light colors, so initials stay readable
    // on whatever color was picked.
    QString textColor(const QString &color);

    // Up to two initials, e.g. "Grace Adeyemi" -> "GA", "Manu" -> "M".
    QString initials(const QString &name);
}

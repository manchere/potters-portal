#pragma once

#include <QList>
#include <QPair>
#include <QString>

// The colors a Member can pick for their profile circle (shown with their
// initials wherever a Member appears). All are dark enough for white text.
// Kept in step with mobile/src/api/memberColors.ts and migration 0022.
namespace MemberColors
{
    // (name, "#rrggbb") in display order.
    const QList<QPair<QString, QString>> &palette();

    QString defaultColor();

    // True when `color` is one of the palette's hex values (any case).
    bool isValid(const QString &color);

    // Up to two initials, e.g. "Grace Adeyemi" -> "GA", "Manu" -> "M".
    QString initials(const QString &name);
}

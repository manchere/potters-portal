#pragma once

#include <QColor>
#include <QString>

#include "Models/Item.h"

class QWidget;
class QLabel;

// Small colored "pill" label shared by the Items table (status column) and
// the card grid (status pill) so both views render statuses identically.
namespace Badge
{
    QColor statusColor(ItemStatus status);

    // Light tint of `color` as background, full `color` as text.
    QLabel *make(const QString &text, const QColor &color, QWidget *parent);
}

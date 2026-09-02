#include "Badge.h"

#include <QLabel>

namespace Badge
{
    QColor statusColor(ItemStatus status)
    {
        switch (status) {
        case ItemStatus::Available: return QColor(0x1f, 0x8a, 0x4c);
        case ItemStatus::Missing: return QColor(0xb4, 0x8a, 0x00);
        case ItemStatus::Broken: return QColor(0xc0, 0x39, 0x2b);
        case ItemStatus::Lost: return QColor(0x6b, 0x72, 0x80);
        }
        return QColor(0x2f, 0x35, 0x42);
    }

    QLabel *make(const QString &text, const QColor &color, QWidget *parent)
    {
        auto *badge = new QLabel(text, parent);
        badge->setAlignment(Qt::AlignCenter);
        QColor tint = color;
        tint.setAlpha(30);
        badge->setStyleSheet(QStringLiteral(
            "QLabel { background: rgba(%1,%2,%3,%4); color: %5; border-radius: 9px; "
            "padding: 2px 10px; font-weight: 600; font-size: 9pt; }")
            .arg(color.red()).arg(color.green()).arg(color.blue()).arg(tint.alpha())
            .arg(color.name()));
        return badge;
    }
}

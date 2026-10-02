#include "MemberBadge.h"

#include <QLabel>

#include "Models/MemberColors.h"

namespace MemberBadge
{
    QLabel *make(const QString &name, const QString &color, int sizePx, QWidget *parent)
    {
        auto *badge = new QLabel(parent);
        badge->setFixedSize(sizePx, sizePx);
        badge->setAlignment(Qt::AlignCenter);
        update(badge, name, color);
        return badge;
    }

    void update(QLabel *badge, const QString &name, const QString &color)
    {
        const int size = badge->width();
        const QString fill = MemberColors::isValid(color) ? color : MemberColors::defaultColor();
        badge->setText(MemberColors::initials(name));
        badge->setStyleSheet(QStringLiteral(
            "QLabel { background: %1; color: white; border-radius: %2px; font-weight: 700; font-size: %3px; }")
            .arg(fill).arg(size / 2).arg(qMax(10, size * 2 / 5)));
    }
}

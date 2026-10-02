#include "MemberColors.h"

#include <QStringList>

namespace MemberColors
{
    const QList<QPair<QString, QString>> &palette()
    {
        static const QList<QPair<QString, QString>> colors = {
            {QStringLiteral("Navy"), QStringLiteral("#1f4a85")},
            {QStringLiteral("Teal"), QStringLiteral("#0f766e")},
            {QStringLiteral("Green"), QStringLiteral("#2f855a")},
            {QStringLiteral("Gold"), QStringLiteral("#b7791f")},
            {QStringLiteral("Orange"), QStringLiteral("#c05621")},
            {QStringLiteral("Red"), QStringLiteral("#c53030")},
            {QStringLiteral("Pink"), QStringLiteral("#b83280")},
            {QStringLiteral("Purple"), QStringLiteral("#6b46c1")},
            {QStringLiteral("Indigo"), QStringLiteral("#4c51bf")},
            {QStringLiteral("Slate"), QStringLiteral("#4a5568")},
        };
        return colors;
    }

    QString defaultColor()
    {
        return palette().first().second;
    }

    bool isValid(const QString &color)
    {
        for (const auto &entry : palette()) {
            if (entry.second.compare(color, Qt::CaseInsensitive) == 0) {
                return true;
            }
        }
        return false;
    }

    QString initials(const QString &name)
    {
        const QStringList words = name.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        QString result;
        for (const QString &word : words) {
            result += word.at(0).toUpper();
            if (result.size() == 2) {
                break;
            }
        }
        return result;
    }
}

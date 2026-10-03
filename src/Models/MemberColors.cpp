#include "MemberColors.h"

#include <QRegularExpression>
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
        static const QRegularExpression pattern(QStringLiteral("^#[0-9a-fA-F]{6}$"));
        return pattern.match(color).hasMatch();
    }

    QString textColor(const QString &color)
    {
        if (!isValid(color)) {
            return QStringLiteral("white");
        }
        // Perceived brightness; above ~0.6 white text washes out. Parsed by
        // hand since the server doesn't link QtGui (no QColor).
        const auto channel = [&color](int index) { return color.mid(1 + index * 2, 2).toInt(nullptr, 16); };
        const double luminance = (0.299 * channel(0) + 0.587 * channel(1) + 0.114 * channel(2)) / 255.0;
        return luminance > 0.6 ? QStringLiteral("#1a202c") : QStringLiteral("white");
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

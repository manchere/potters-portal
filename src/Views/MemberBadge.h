#pragma once

#include <QString>

class QLabel;
class QWidget;

// A Member's profile circle: their initials in white on their chosen color
// (see Models/MemberColors). Used wherever a Member appears -- the
// Taxonomy list, Schedule rows, the responsibilities dialog, and the
// Add/Edit Member preview.
namespace MemberBadge
{
    QLabel *make(const QString &name, const QString &color, int sizePx, QWidget *parent);

    // Re-colors / re-letters an existing badge, e.g. as the name is typed.
    void update(QLabel *badge, const QString &name, const QString &color);
}

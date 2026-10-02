#include "MemberColorPicker.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QToolButton>

#include "Models/MemberColors.h"

namespace
{
    constexpr int kSwatchSize = 26;
}

MemberColorPicker::MemberColorPicker(QWidget *parent)
    : QWidget(parent)
{
    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    const auto &palette = MemberColors::palette();
    for (int i = 0; i < palette.size(); ++i) {
        auto *swatch = new QToolButton(this);
        swatch->setCheckable(true);
        swatch->setFixedSize(kSwatchSize, kSwatchSize);
        swatch->setCursor(Qt::PointingHandCursor);
        swatch->setToolTip(palette.at(i).first);
        swatch->setProperty("color", palette.at(i).second);
        // The selected swatch gets a ring in the text color, so it shows in
        // either theme.
        swatch->setStyleSheet(QStringLiteral(
            "QToolButton { background: %1; border: 2px solid transparent; border-radius: %2px; }"
            "QToolButton:checked { border: 3px solid palette(text); }")
            .arg(palette.at(i).second).arg(kSwatchSize / 2));
        m_group->addButton(swatch, i);
        layout->addWidget(swatch);
    }
    layout->addStretch();

    connect(m_group, &QButtonGroup::idClicked, this, [this]() { emit colorChanged(color()); });
    setColor(MemberColors::defaultColor());
}

QString MemberColorPicker::color() const
{
    const QAbstractButton *checked = m_group->checkedButton();
    return checked ? checked->property("color").toString() : MemberColors::defaultColor();
}

void MemberColorPicker::setColor(const QString &color)
{
    const QString wanted = MemberColors::isValid(color) ? color : MemberColors::defaultColor();
    for (QAbstractButton *button : m_group->buttons()) {
        if (button->property("color").toString().compare(wanted, Qt::CaseInsensitive) == 0) {
            button->setChecked(true);
            break;
        }
    }
}

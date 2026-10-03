#include "MemberColorPicker.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QColorDialog>
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

    m_customSwatch = new QToolButton(this);
    m_customSwatch->setCheckable(true);
    m_customSwatch->setFixedSize(kSwatchSize, kSwatchSize);
    m_customSwatch->setCursor(Qt::PointingHandCursor);
    m_customSwatch->setToolTip(tr("Any color..."));
    m_customSwatch->setText(QStringLiteral("+"));
    setCustomColor(QString());
    m_group->addButton(m_customSwatch, palette.size());
    layout->addWidget(m_customSwatch);
    layout->addStretch();

    connect(m_group, &QButtonGroup::idClicked, this, [this](int id) {
        if (m_group->button(id) == m_customSwatch) {
            chooseCustomColor();
            return;
        }
        m_previousColor = color();
        emit colorChanged(color());
    });
    setColor(MemberColors::defaultColor());
}

void MemberColorPicker::chooseCustomColor()
{
    const QColor chosen = QColorDialog::getColor(QColor(m_previousColor), this, tr("Choose a Color"));
    if (!chosen.isValid()) {
        setColor(m_previousColor); // cancelled: back to what was picked before
        return;
    }
    setColor(chosen.name());
    emit colorChanged(color());
}

void MemberColorPicker::setCustomColor(const QString &color)
{
    m_customSwatch->setProperty("color", color);
    // Until a color is chosen it's an outlined "+"; after, it shows that
    // color with the "+" in a readable shade.
    const QString fill = color.isEmpty() ? QStringLiteral("transparent") : color;
    const QString text = color.isEmpty() ? QStringLiteral("palette(text)") : MemberColors::textColor(color);
    m_customSwatch->setStyleSheet(QStringLiteral(
        "QToolButton { background: %1; color: %2; font-weight: 700; border: 2px dashed palette(mid); border-radius: %3px; }"
        "QToolButton:checked { border: 3px solid palette(text); }")
        .arg(fill, text).arg(kSwatchSize / 2));
}

QString MemberColorPicker::color() const
{
    const QAbstractButton *checked = m_group->checkedButton();
    return checked ? checked->property("color").toString() : MemberColors::defaultColor();
}

void MemberColorPicker::setColor(const QString &color)
{
    const QString wanted = (MemberColors::isValid(color) ? color : MemberColors::defaultColor()).toLower();
    m_previousColor = wanted;
    for (QAbstractButton *button : m_group->buttons()) {
        if (button != m_customSwatch && button->property("color").toString().compare(wanted, Qt::CaseInsensitive) == 0) {
            button->setChecked(true);
            return;
        }
    }
    setCustomColor(wanted);
    m_customSwatch->setChecked(true);
}

#include "Sidebar.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QVBoxLayout>

Sidebar::Sidebar(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("sidebar"));
    setFixedWidth(184);

    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);
    connect(m_group, &QButtonGroup::idClicked, this, &Sidebar::currentChanged);

    m_buttonLayout = new QVBoxLayout;
    m_buttonLayout->setContentsMargins(0, 0, 0, 0);
    m_buttonLayout->setSpacing(4);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 14, 10, 14);
    layout->setSpacing(0);
    layout->addLayout(m_buttonLayout);
    layout->addStretch();
}

void Sidebar::addPage(const QString &icon, const QString &label)
{
    auto *button = new QPushButton(this);
    button->setObjectName(QStringLiteral("sidebarButton"));
    button->setText(QStringLiteral("%1   %2").arg(icon, label));
    button->setToolTip(label);
    button->setCheckable(true);
    button->setCursor(Qt::PointingHandCursor);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    const int index = m_group->buttons().size();
    m_group->addButton(button, index);
    m_buttonLayout->addWidget(button);
    if (index == 0) {
        button->setChecked(true);
    }
}

int Sidebar::currentIndex() const
{
    return m_group->checkedId();
}

void Sidebar::setCurrentIndex(int index)
{
    QAbstractButton *button = m_group->button(index);
    if (!button || button->isChecked()) {
        return;
    }
    button->setChecked(true);
    emit currentChanged(index);
}

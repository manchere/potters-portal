#include "ActionBar.h"

#include <QEvent>
#include <QLabel>
#include <QVBoxLayout>

ActionBar::ActionBar(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("actionBar"));
    setFixedWidth(196);

    auto *heading = new QLabel(QStringLiteral("Actions"), this);
    heading->setObjectName(QStringLiteral("actionBarHeading"));

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(12, 12, 12, 12);
    m_layout->setSpacing(8);
    m_layout->addWidget(heading);
}

void ActionBar::addWidget(QWidget *widget)
{
    widget->setParent(this);
    widget->setSizePolicy(QSizePolicy::Expanding, widget->sizePolicy().verticalPolicy());
    if (auto *label = qobject_cast<QLabel *>(widget)) {
        label->setWordWrap(true);
    }
    widget->installEventFilter(this);
    m_widgets.append(widget);
    m_layout->addWidget(widget);
    updateVisibility();
}

void ActionBar::addSeparator()
{
    auto *line = new QFrame(this);
    line->setObjectName(QStringLiteral("actionBarSeparator"));
    line->setFixedHeight(1);
    m_layout->addSpacing(2);
    m_layout->addWidget(line);
    m_layout->addSpacing(2);
}

void ActionBar::addStretch()
{
    m_layout->addStretch(1);
}

bool ActionBar::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::ShowToParent || event->type() == QEvent::HideToParent) {
        updateVisibility();
    }
    return QFrame::eventFilter(watched, event);
}

void ActionBar::updateVisibility()
{
    bool anyVisible = false;
    for (QWidget *widget : std::as_const(m_widgets)) {
        if (!widget->isHidden()) {
            anyVisible = true;
            break;
        }
    }
    setVisible(anyVisible);
}

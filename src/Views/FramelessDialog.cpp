#include "FramelessDialog.h"

#include <QMouseEvent>
#include <QScreen>
#include <QVBoxLayout>
#include <QWindow>

FramelessDialog::FramelessDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    // Overridden to "background: transparent" in Style.cpp -- without this
    // name, the app's generic "QDialog { background: #f7f8fb; }" rule
    // would paint this (translucent) top-level widget opaque and defeat
    // the whole effect.
    setObjectName(QStringLiteral("framelessDialogRoot"));

    m_card = new QWidget(this);
    m_card->setObjectName(QStringLiteral("dialogCard"));
    m_card->setAttribute(Qt::WA_StyledBackground, true);
    m_cardLayout = new QVBoxLayout(m_card);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(m_card);
}

void FramelessDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    // parentWidget()->geometry() would be in the *parent's own parent*
    // coordinate system when parentWidget() isn't itself a top-level
    // window (e.g. a tab page) -- window() walks up to the actual
    // top-level widget, whose geometry is in real screen coordinates.
    const QWidget *anchorWidget = parentWidget() ? parentWidget()->window() : nullptr;
    const QRect anchor = anchorWidget ? anchorWidget->geometry()
        : (screen() ? screen()->availableGeometry() : QRect());
    if (!anchor.isNull()) {
        move(anchor.center() - QPoint(width() / 2, height() / 2));
    }
}

void FramelessDialog::mousePressEvent(QMouseEvent *event)
{
    // No title bar to drag by -- let the whole dialog be dragged from any
    // empty area, mirroring MainWindow's own frameless-drag behavior.
    if (event->button() == Qt::LeftButton) {
        if (QWindow *handle = windowHandle()) {
            handle->startSystemMove();
            event->accept();
            return;
        }
    }
    QDialog::mousePressEvent(event);
}

#include "ClickableLabel.h"

#include <QMouseEvent>

ClickableLabel::ClickableLabel(QWidget *parent)
    : QLabel(parent)
{
    setCursor(Qt::PointingHandCursor);
}

void ClickableLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // Accepted so the press doesn't also reach a parent -- e.g. a
        // FramelessDialog, which would start dragging the window.
        event->accept();
        emit clicked();
        return;
    }
    QLabel::mousePressEvent(event);
}

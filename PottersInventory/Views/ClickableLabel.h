#pragma once

#include <QLabel>

// A QLabel that emits clicked() on a left mouse press, with a pointing-hand
// cursor. Used for the photo tile so the tile itself is the "choose photo"
// control instead of a separate button next to it.
class ClickableLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ClickableLabel(QWidget *parent = nullptr);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
};

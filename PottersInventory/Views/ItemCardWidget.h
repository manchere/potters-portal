#pragma once

#include <QWidget>

#include "Models/Item.h"
#include "Models/Tag.h"

class QPixmap;

// One product-style card in the Items grid view: photo, name, status pill,
// quantity, and a small colored dot per tag (tag names shown as tooltips
// to keep the card compact). Double-click opens the read-only details
// modal (ItemListView owns that wiring); other actions (edit/delete/etc.)
// stay on the list view's button row.
class ItemCardWidget : public QWidget
{
    Q_OBJECT

public:
    ItemCardWidget(const Item &item, const QString &categoryName, const QVector<Tag> &tags,
                    const QPixmap &photo, QWidget *parent = nullptr);

    int itemId() const { return m_itemId; }

signals:
    void doubleClicked(int itemId);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    int m_itemId = -1;
};

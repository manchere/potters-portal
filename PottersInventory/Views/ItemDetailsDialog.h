#pragma once

#include "FramelessDialog.h"

#include <QPixmap>
#include <QVector>

#include "Models/Item.h"
#include "Models/Tag.h"

// Read-only "quick view" modal opened by double-clicking an item in the
// list or card view — shows every field at a glance without opening the
// full edit form. Editing stays on the explicit Edit button/dialog.
class ItemDetailsDialog : public FramelessDialog
{
    Q_OBJECT

public:
    ItemDetailsDialog(const Item &item, const QString &categoryName, const QVector<Tag> &tags,
                       const QPixmap &photo, QWidget *parent = nullptr);
};

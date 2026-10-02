#pragma once

#include "FramelessDialog.h"

class ItemController;
class TagController;
class CategoryController;
class ItemFormWidget;

// Modal "Add Item" dialog opened from the Items tab (next to the page
// title). Reuses ItemFormWidget with a blank form, saving via
// ItemController::addItem — mirrors ItemEditDialog but for creation instead
// of updating an existing item.
class ItemAddDialog : public FramelessDialog
{
    Q_OBJECT

public:
    ItemAddDialog(ItemController *itemController, TagController *tagController,
                  CategoryController *categoryController, QWidget *parent = nullptr);

private slots:
    void saveClicked();

private:
    ItemController *m_itemController = nullptr;
    ItemFormWidget *m_form = nullptr;
};

#pragma once

#include "FramelessDialog.h"

#include "Models/Item.h"

class ItemController;
class TagController;
class CategoryController;
class ItemFormWidget;

// Modal "Edit Item" dialog opened from the item list. Reuses ItemFormWidget
// pre-filled with the selected item, saving via ItemController::updateItem.
class ItemEditDialog : public FramelessDialog
{
    Q_OBJECT

public:
    ItemEditDialog(const Item &item, ItemController *itemController, TagController *tagController,
                    CategoryController *categoryController, QWidget *parent = nullptr);

private slots:
    void saveClicked();

private:
    ItemController *m_itemController = nullptr;
    ItemFormWidget *m_form = nullptr;
};

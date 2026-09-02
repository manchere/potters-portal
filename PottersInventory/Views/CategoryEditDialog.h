#pragma once

#include "FramelessDialog.h"

#include "Models/Category.h"

class QLineEdit;
class QPlainTextEdit;

// Add/rename a category: name + description. Used by TagsCategoriesView for
// both "Add Category" and "Rename Category" so the two flows share one
// implementation.
class CategoryEditDialog : public FramelessDialog
{
    Q_OBJECT

public:
    // Pass an existing category to pre-fill for editing, or a
    // default-constructed Category() for "add new".
    CategoryEditDialog(const Category &category, QWidget *parent = nullptr);

    Category category() const;

private:
    int m_id = -1;
    QLineEdit *m_nameEdit = nullptr;
    QPlainTextEdit *m_descriptionEdit = nullptr;
};

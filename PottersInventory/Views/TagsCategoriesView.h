#pragma once

#include <QHash>
#include <QWidget>

class QListWidget;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QLabel;
class TagController;
class CategoryController;

// "Tags & Categories" tab. Tags are fully managed inline in this one view,
// no popup dialogs: tags are shown as colored chips (background = the tag's
// own color) that wrap onto new rows as they fill up, rather than a plain
// column list. The Name/Color/Description form below creates a new tag when
// nothing is selected, and clicking a chip selects/autofills that same form
// for editing -- the Save button then updates it instead of creating
// another one. Categories still use the separate CategoryEditDialog (not
// asked to change).
class TagsCategoriesView : public QWidget
{
    Q_OBJECT

public:
    TagsCategoriesView(TagController *tagController, CategoryController *categoryController, QWidget *parent = nullptr);

public slots:
    void refreshTags();
    void refreshCategories();

private slots:
    void saveTagClicked();
    void pickTagColor();
    void newTagClicked();
    void deleteTagClicked();
    void addCategoryClicked();
    void renameCategoryClicked();
    void deleteCategoryClicked();

private:
    void updateTagColorSwatch();
    void resetTagForm();
    void selectTag(int tagId);
    QLabel *makeTagChip(int tagId, const QString &name, const QString &color, const QString &description);
    void applyChipStyle(QLabel *chip, const QString &color, bool selected);

    TagController *m_tagController = nullptr;
    CategoryController *m_categoryController = nullptr;
    QWidget *m_tagFlowContainer = nullptr;
    QListWidget *m_categoryList = nullptr;
    QHash<int, QLabel *> m_tagChipsById;

    // Inline tag create/edit form, embedded in the Tags box. -1 means the
    // form is in "create a new tag" mode; otherwise it's editing that tag
    // (and that tag's chip is shown selected).
    int m_editingTagId = -1;
    QLineEdit *m_tagNameEdit = nullptr;
    QPushButton *m_tagColorButton = nullptr;
    QPlainTextEdit *m_tagDescriptionEdit = nullptr;
    QPushButton *m_saveTagButton = nullptr;
    QString m_tagColor;
};

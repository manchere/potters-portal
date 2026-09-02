#include "TagsCategoriesView.h"

#include <QColor>
#include <QColorDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "Controllers/CategoryController.h"
#include "Controllers/TagController.h"
#include "CategoryEditDialog.h"
#include "ClickableLabel.h"
#include "FlowLayout.h"

static QListWidget *makeList(QWidget *parent)
{
    auto *list = new QListWidget(parent);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    return list;
}

TagsCategoriesView::TagsCategoriesView(TagController *tagController, CategoryController *categoryController, QWidget *parent)
    : QWidget(parent)
    , m_tagController(tagController)
    , m_categoryController(categoryController)
{
    auto *title = new QLabel(QStringLiteral("Tags & Categories"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(QStringLiteral("Manage the tags and categories items can be assigned to."), this);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));

    // Tags render as colored chips (background = the tag's own color) that
    // wrap onto a new row once the current one fills up, rather than a
    // plain column list -- see makeTagChip()/refreshTags().
    m_tagFlowContainer = new QWidget(this);
    new FlowLayout(m_tagFlowContainer, 4, 8, 8);
    m_tagFlowContainer->setMinimumHeight(80);

    auto *newTagButton = new QPushButton(QStringLiteral("New Tag"), this);
    newTagButton->setObjectName(QStringLiteral("secondaryButton"));
    newTagButton->setToolTip(QStringLiteral("Clear the form below to create a tag instead of editing the selected one"));
    auto *deleteTagButton = new QPushButton(QStringLiteral("Delete"), this);
    deleteTagButton->setObjectName(QStringLiteral("dangerButton"));
    connect(newTagButton, &QPushButton::clicked, this, &TagsCategoriesView::newTagClicked);
    connect(deleteTagButton, &QPushButton::clicked, this, &TagsCategoriesView::deleteTagClicked);

    auto *tagButtons = new QHBoxLayout;
    tagButtons->addWidget(newTagButton);
    tagButtons->addWidget(deleteTagButton);

    // --- Inline create/edit form -- selecting a tag above autofills this
    // for editing; Save creates or updates depending on m_editingTagId. No
    // separate popup dialog for either flow. ---
    m_tagNameEdit = new QLineEdit(this);
    m_tagNameEdit->setPlaceholderText(QStringLiteral("e.g. Electronics"));
    m_tagColor = QStringLiteral("#3b82f6");
    m_tagColorButton = new QPushButton(this);
    m_tagColorButton->setFixedWidth(70);
    connect(m_tagColorButton, &QPushButton::clicked, this, &TagsCategoriesView::pickTagColor);
    updateTagColorSwatch();
    m_tagDescriptionEdit = new QPlainTextEdit(this);
    m_tagDescriptionEdit->setFixedHeight(50);
    m_tagDescriptionEdit->setPlaceholderText(QStringLiteral("Optional notes about what this tag means"));
    m_saveTagButton = new QPushButton(QStringLiteral("Create Tag"), this);
    connect(m_saveTagButton, &QPushButton::clicked, this, &TagsCategoriesView::saveTagClicked);

    auto *tagForm = new QFormLayout;
    tagForm->addRow(QStringLiteral("Name"), m_tagNameEdit);
    tagForm->addRow(QStringLiteral("Color"), m_tagColorButton);
    tagForm->addRow(QStringLiteral("Description"), m_tagDescriptionEdit);

    auto *tagLayout = new QVBoxLayout;
    tagLayout->addWidget(m_tagFlowContainer);
    tagLayout->addLayout(tagButtons);
    tagLayout->addSpacing(8);
    tagLayout->addLayout(tagForm);
    tagLayout->addWidget(m_saveTagButton);
    auto *tagBox = new QGroupBox(QStringLiteral("Tags"), this);
    tagBox->setLayout(tagLayout);

    m_categoryList = makeList(this);
    m_categoryList->setAlternatingRowColors(true);
    auto *addCategoryButton = new QPushButton(QStringLiteral("Add"), this);
    auto *renameCategoryButton = new QPushButton(QStringLiteral("Rename"), this);
    renameCategoryButton->setObjectName(QStringLiteral("secondaryButton"));
    auto *deleteCategoryButton = new QPushButton(QStringLiteral("Delete"), this);
    deleteCategoryButton->setObjectName(QStringLiteral("dangerButton"));
    connect(addCategoryButton, &QPushButton::clicked, this, &TagsCategoriesView::addCategoryClicked);
    connect(renameCategoryButton, &QPushButton::clicked, this, &TagsCategoriesView::renameCategoryClicked);
    connect(deleteCategoryButton, &QPushButton::clicked, this, &TagsCategoriesView::deleteCategoryClicked);

    auto *categoryButtons = new QHBoxLayout;
    categoryButtons->addWidget(addCategoryButton);
    categoryButtons->addWidget(renameCategoryButton);
    categoryButtons->addWidget(deleteCategoryButton);

    auto *categoryLayout = new QVBoxLayout;
    categoryLayout->addWidget(m_categoryList);
    categoryLayout->addLayout(categoryButtons);
    auto *categoryBox = new QGroupBox(QStringLiteral("Categories"), this);
    categoryBox->setLayout(categoryLayout);

    auto *columns = new QHBoxLayout;
    columns->setSpacing(16);
    columns->addWidget(tagBox);
    columns->addWidget(categoryBox);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(6);
    layout->addLayout(columns);

    refreshTags();
    refreshCategories();
}

void TagsCategoriesView::refreshTags()
{
    const auto oldChips = m_tagFlowContainer->findChildren<QLabel *>(QString(), Qt::FindDirectChildrenOnly);
    qDeleteAll(oldChips);
    m_tagChipsById.clear();

    auto *flow = static_cast<FlowLayout *>(m_tagFlowContainer->layout());
    for (const Tag &tag : m_tagController->allTags()) {
        QLabel *chip = makeTagChip(tag.id(), tag.name(), tag.color(), tag.description());
        m_tagChipsById.insert(tag.id(), chip);
        flow->addWidget(chip);
    }
}

QLabel *TagsCategoriesView::makeTagChip(int tagId, const QString &name, const QString &color, const QString &description)
{
    auto *chip = new ClickableLabel(m_tagFlowContainer);
    chip->setText(name);
    chip->setAlignment(Qt::AlignCenter);
    if (!description.isEmpty()) {
        chip->setToolTip(description);
    }
    applyChipStyle(chip, color, tagId == m_editingTagId);
    connect(chip, &ClickableLabel::clicked, this, [this, tagId]() { selectTag(tagId); });
    return chip;
}

void TagsCategoriesView::applyChipStyle(QLabel *chip, const QString &color, bool selected)
{
    const QColor background(color);
    const QString textColor = background.lightness() < 140 ? QStringLiteral("white") : QStringLiteral("#1f2430");
    chip->setStyleSheet(QStringLiteral(
        "QLabel { background: %1; color: %2; border-radius: 12px; padding: 6px 14px; "
        "font-weight: 600; border: 2px solid %3; }")
        .arg(color, textColor, selected ? QStringLiteral("#14335c") : color));
}

void TagsCategoriesView::refreshCategories()
{
    m_categoryList->clear();
    for (const Category &category : m_categoryController->allCategories()) {
        auto *item = new QListWidgetItem(category.name(), m_categoryList);
        item->setData(Qt::UserRole, category.id());
        if (!category.description().isEmpty()) {
            item->setToolTip(category.description());
        }
    }
}

void TagsCategoriesView::saveTagClicked()
{
    const QString name = m_tagNameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Save Tag"), QStringLiteral("Name is required."));
        return;
    }
    Tag tag(m_editingTagId, name, m_tagColor, m_tagDescriptionEdit->toPlainText().trimmed());
    const bool ok = m_editingTagId < 0 ? m_tagController->addTag(tag) : m_tagController->updateTag(tag);
    if (ok) {
        resetTagForm();
        refreshTags();
    } else {
        QMessageBox::critical(this, QStringLiteral("Save Tag"), m_tagController->lastError());
    }
}

void TagsCategoriesView::pickTagColor()
{
    const QColor chosen = QColorDialog::getColor(QColor(m_tagColor), this, QStringLiteral("Tag Color"));
    if (chosen.isValid()) {
        m_tagColor = chosen.name();
        updateTagColorSwatch();
    }
}

void TagsCategoriesView::updateTagColorSwatch()
{
    m_tagColorButton->setText(m_tagColor);
    m_tagColorButton->setStyleSheet(QStringLiteral("background-color: %1; color: white;").arg(m_tagColor));
}

// Clicking a tag's chip autofills the form below for editing; the Save
// button then updates that tag instead of creating a new one.
void TagsCategoriesView::selectTag(int tagId)
{
    const Tag tag = m_tagController->tagById(tagId);

    if (QLabel *previousChip = m_tagChipsById.value(m_editingTagId)) {
        applyChipStyle(previousChip, m_tagController->tagById(m_editingTagId).color(), false);
    }
    m_editingTagId = tag.id();
    if (QLabel *chip = m_tagChipsById.value(m_editingTagId)) {
        applyChipStyle(chip, tag.color(), true);
    }

    m_tagNameEdit->setText(tag.name());
    m_tagColor = tag.color();
    updateTagColorSwatch();
    m_tagDescriptionEdit->setPlainText(tag.description());
    m_saveTagButton->setText(QStringLiteral("Save Changes"));
}

void TagsCategoriesView::newTagClicked()
{
    resetTagForm();
}

void TagsCategoriesView::resetTagForm()
{
    if (QLabel *previousChip = m_tagChipsById.value(m_editingTagId)) {
        applyChipStyle(previousChip, m_tagController->tagById(m_editingTagId).color(), false);
    }
    m_editingTagId = -1;
    m_tagNameEdit->clear();
    m_tagColor = QStringLiteral("#3b82f6");
    updateTagColorSwatch();
    m_tagDescriptionEdit->clear();
    m_saveTagButton->setText(QStringLiteral("Create Tag"));
}

void TagsCategoriesView::deleteTagClicked()
{
    if (m_editingTagId < 0) {
        QMessageBox::information(this, QStringLiteral("Delete Tag"), QStringLiteral("Select a tag first."));
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("Delete Tag"), QStringLiteral("Delete this tag?")) != QMessageBox::Yes) {
        return;
    }
    const int id = m_editingTagId;
    if (m_tagController->removeTag(id)) {
        resetTagForm();
        refreshTags();
    } else {
        QMessageBox::critical(this, QStringLiteral("Delete Tag"), m_tagController->lastError());
    }
}

void TagsCategoriesView::addCategoryClicked()
{
    CategoryEditDialog dialog(Category(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    Category newCategory = dialog.category();
    if (newCategory.name().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Add Category"), QStringLiteral("Name is required."));
        return;
    }
    if (m_categoryController->addCategory(newCategory)) {
        refreshCategories();
    } else {
        QMessageBox::critical(this, QStringLiteral("Add Category"), m_categoryController->lastError());
    }
}

void TagsCategoriesView::renameCategoryClicked()
{
    QListWidgetItem *selected = m_categoryList->currentItem();
    if (!selected) {
        QMessageBox::information(this, QStringLiteral("Rename Category"), QStringLiteral("Select a category first."));
        return;
    }
    const Category existing = m_categoryController->categoryById(selected->data(Qt::UserRole).toInt());
    CategoryEditDialog dialog(existing, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    Category category = dialog.category();
    if (category.name().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Rename Category"), QStringLiteral("Name is required."));
        return;
    }
    if (m_categoryController->updateCategory(category)) {
        refreshCategories();
    } else {
        QMessageBox::critical(this, QStringLiteral("Rename Category"), m_categoryController->lastError());
    }
}

void TagsCategoriesView::deleteCategoryClicked()
{
    QListWidgetItem *selected = m_categoryList->currentItem();
    if (!selected) {
        QMessageBox::information(this, QStringLiteral("Delete Category"), QStringLiteral("Select a category first."));
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("Delete Category"), QStringLiteral("Delete this category?")) != QMessageBox::Yes) {
        return;
    }
    if (m_categoryController->removeCategory(selected->data(Qt::UserRole).toInt())) {
        refreshCategories();
    } else {
        QMessageBox::critical(this, QStringLiteral("Delete Category"), m_categoryController->lastError());
    }
}

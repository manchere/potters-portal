#include "ItemAddDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "Controllers/ItemController.h"
#include "ItemFormWidget.h"

ItemAddDialog::ItemAddDialog(ItemController *itemController, TagController *tagController,
                              CategoryController *categoryController, QWidget *parent)
    : FramelessDialog(parent)
    , m_itemController(itemController)
{
    setWindowTitle(QStringLiteral("Add Item"));
    auto *heading = new QLabel(QStringLiteral("Add Item"), this);
    heading->setObjectName(QStringLiteral("pageTitle"));

    m_form = new ItemFormWidget(tagController, categoryController, /*autoFillLocation=*/true, this);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Save)->setText(QStringLiteral("Add Item"));
    connect(buttons, &QDialogButtonBox::accepted, this, &ItemAddDialog::saveClicked);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(heading);
    layout->addWidget(m_form);
    layout->addWidget(buttons);
}

void ItemAddDialog::saveClicked()
{
    Item item = m_form->toItem();
    if (item.name().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Add Item"), QStringLiteral("Name is required."));
        return;
    }
    if (!m_itemController->addItem(item)) {
        QMessageBox::critical(this, QStringLiteral("Add Item"), m_itemController->lastError());
        return;
    }
    if (m_form->imageChanged() && !m_itemController->setItemImage(item.id(), m_form->imageData(), m_form->imageMime())) {
        QMessageBox::warning(this, QStringLiteral("Add Item"),
                              QStringLiteral("Item was saved, but the photo could not be attached: %1")
                                  .arg(m_itemController->lastError()));
    }
    accept();
}

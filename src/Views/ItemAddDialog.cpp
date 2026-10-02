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
    setWindowTitle(tr("Add Item"));
    auto *heading = new QLabel(tr("Add Item"), this);
    heading->setObjectName(QStringLiteral("pageTitle"));

    m_form = new ItemFormWidget(tagController, categoryController, /*autoFillLocation=*/true, this);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Save)->setText(tr("Add Item"));
    connect(buttons, &QDialogButtonBox::accepted, this, &ItemAddDialog::saveClicked);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = contentLayout();
    layout->addWidget(heading);
    layout->addWidget(m_form);
    layout->addWidget(buttons);
}

void ItemAddDialog::saveClicked()
{
    if (!m_form->validate()) {
        return;
    }
    Item item = m_form->toItem();
    if (!m_itemController->addItem(item)) {
        QMessageBox::critical(this, tr("Add Item"), m_itemController->lastError());
        return;
    }
    if (m_form->imageChanged() && !m_itemController->setItemImage(item.id(), m_form->imageData(), m_form->imageMime())) {
        QMessageBox::warning(this, tr("Add Item"),
                              tr("Item was saved, but the photo could not be attached: %1")
                                  .arg(m_itemController->lastError()));
    }
    accept();
}

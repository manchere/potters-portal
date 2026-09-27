#include "ItemEditDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

#include "Controllers/ItemController.h"
#include "ItemFormWidget.h"

ItemEditDialog::ItemEditDialog(const Item &item, ItemController *itemController, TagController *tagController,
                                CategoryController *categoryController, QWidget *parent)
    : FramelessDialog(parent)
    , m_itemController(itemController)
{
    setWindowTitle(QStringLiteral("Edit Item"));
    auto *heading = new QLabel(QStringLiteral("Edit Item"), this);
    heading->setObjectName(QStringLiteral("pageTitle"));

    m_form = new ItemFormWidget(tagController, categoryController, /*autoFillLocation=*/false, this);
    m_form->setItem(item);

    if (!item.imageMime().isEmpty()) {
        QByteArray data;
        QString mime;
        if (itemController->itemImage(item.id(), data, mime)) {
            m_form->setExistingImage(data, mime);
        }
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &ItemEditDialog::saveClicked);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = contentLayout();
    layout->addWidget(heading);
    layout->addWidget(m_form);
    layout->addWidget(buttons);
}

void ItemEditDialog::saveClicked()
{
    if (!m_form->validate()) {
        return;
    }
    Item item = m_form->toItem();
    if (!m_itemController->updateItem(item)) {
        QMessageBox::critical(this, QStringLiteral("Edit Item"), m_itemController->lastError());
        return;
    }
    if (m_form->imageChanged() && !m_itemController->setItemImage(item.id(), m_form->imageData(), m_form->imageMime())) {
        QMessageBox::warning(this, QStringLiteral("Edit Item"),
                              QStringLiteral("Item was saved, but the photo could not be updated: %1")
                                  .arg(m_itemController->lastError()));
    }
    accept();
}

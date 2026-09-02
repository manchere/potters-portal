#include "CategoryEditDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QVBoxLayout>

CategoryEditDialog::CategoryEditDialog(const Category &category, QWidget *parent)
    : FramelessDialog(parent)
    , m_id(category.id())
{
    const QString title = category.id() < 0 ? QStringLiteral("Add Category") : QStringLiteral("Rename Category");
    setWindowTitle(title);
    auto *heading = new QLabel(title, this);
    heading->setObjectName(QStringLiteral("pageTitle"));

    m_nameEdit = new QLineEdit(category.name(), this);
    m_nameEdit->setPlaceholderText(QStringLiteral("e.g. Furniture"));

    m_descriptionEdit = new QPlainTextEdit(category.description(), this);
    m_descriptionEdit->setFixedHeight(60);
    m_descriptionEdit->setPlaceholderText(
        QStringLiteral("Optional notes about what this category is for, e.g. \"Kitchen equipment used for hospitality events\""));

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("Name"), m_nameEdit);
    form->addRow(QStringLiteral("Description"), m_descriptionEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(heading);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

Category CategoryEditDialog::category() const
{
    return Category(m_id, m_nameEdit->text().trimmed(), m_descriptionEdit->toPlainText().trimmed());
}

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
    const QString title = category.id() < 0 ? tr("Add Category") : tr("Rename Category");
    setWindowTitle(title);
    auto *heading = new QLabel(title, this);
    heading->setObjectName(QStringLiteral("pageTitle"));

    m_nameEdit = new QLineEdit(category.name(), this);
    m_nameEdit->setPlaceholderText(tr("e.g. Furniture"));
    connect(m_nameEdit, &QLineEdit::textChanged, this, [this]() { m_nameError->clear(); });
    m_nameError = new QLabel(this);
    m_nameError->setObjectName(QStringLiteral("fieldError"));

    m_descriptionEdit = new QPlainTextEdit(category.description(), this);
    m_descriptionEdit->setFixedHeight(60);
    m_descriptionEdit->setPlaceholderText(
        tr("Optional notes about what this category is for, e.g. \"Kitchen equipment used for hospitality events\""));

    auto *form = new QFormLayout;
    form->addRow(tr("Name"), m_nameEdit);
    form->addRow(QString(), m_nameError);
    form->addRow(tr("Description"), m_descriptionEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &CategoryEditDialog::saveClicked);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = contentLayout();
    layout->addWidget(heading);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

void CategoryEditDialog::saveClicked()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        m_nameError->setText(tr("Name is required."));
        return;
    }
    accept();
}

Category CategoryEditDialog::category() const
{
    return Category(m_id, m_nameEdit->text().trimmed(), m_descriptionEdit->toPlainText().trimmed());
}

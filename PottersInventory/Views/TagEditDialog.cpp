#include "TagEditDialog.h"

#include <QColorDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

TagEditDialog::TagEditDialog(const Tag &tag, QWidget *parent)
    : FramelessDialog(parent)
    , m_id(tag.id())
    , m_color(tag.color().isEmpty() ? QStringLiteral("#3b82f6") : tag.color())
{
    const QString title = tag.id() < 0 ? QStringLiteral("Add Tag") : QStringLiteral("Edit Tag");
    setWindowTitle(title);
    auto *heading = new QLabel(title, this);
    heading->setObjectName(QStringLiteral("pageTitle"));

    m_nameEdit = new QLineEdit(tag.name(), this);
    m_nameEdit->setPlaceholderText(QStringLiteral("e.g. Electronics"));
    connect(m_nameEdit, &QLineEdit::textChanged, this, [this]() { m_nameError->clear(); });
    m_nameError = new QLabel(this);
    m_nameError->setObjectName(QStringLiteral("fieldError"));

    m_colorButton = new QPushButton(this);
    m_colorButton->setFixedWidth(90);
    connect(m_colorButton, &QPushButton::clicked, this, &TagEditDialog::pickColor);
    updateColorSwatch();

    m_descriptionEdit = new QPlainTextEdit(tag.description(), this);
    m_descriptionEdit->setFixedHeight(60);
    m_descriptionEdit->setPlaceholderText(QStringLiteral("Optional notes about what this tag means"));

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("Name"), m_nameEdit);
    form->addRow(QString(), m_nameError);
    form->addRow(QStringLiteral("Color"), m_colorButton);
    form->addRow(QStringLiteral("Description"), m_descriptionEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &TagEditDialog::saveClicked);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = contentLayout();
    layout->addWidget(heading);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

void TagEditDialog::pickColor()
{
    const QColor chosen = QColorDialog::getColor(QColor(m_color), this, QStringLiteral("Tag Color"));
    if (chosen.isValid()) {
        m_color = chosen.name();
        updateColorSwatch();
    }
}

void TagEditDialog::updateColorSwatch()
{
    m_colorButton->setText(m_color);
    m_colorButton->setStyleSheet(QStringLiteral("background-color: %1; color: white;").arg(m_color));
}

void TagEditDialog::saveClicked()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        m_nameError->setText(QStringLiteral("Name is required."));
        return;
    }
    accept();
}

Tag TagEditDialog::tag() const
{
    return Tag(m_id, m_nameEdit->text().trimmed(), m_color, m_descriptionEdit->toPlainText().trimmed());
}

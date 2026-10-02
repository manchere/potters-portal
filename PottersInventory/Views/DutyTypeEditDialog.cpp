#include "DutyTypeEditDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

DutyTypeEditDialog::DutyTypeEditDialog(const DutyType &dutyType, QWidget *parent)
    : FramelessDialog(parent)
    , m_id(dutyType.id())
{
    const QString title = dutyType.id() < 0 ? QStringLiteral("Add Duty Type") : QStringLiteral("Edit Duty Type");
    setWindowTitle(title);
    auto *heading = new QLabel(title, this);
    heading->setObjectName(QStringLiteral("pageTitle"));

    m_nameEdit = new QLineEdit(dutyType.name(), this);
    m_nameEdit->setPlaceholderText(QStringLiteral("e.g. Ushering"));
    connect(m_nameEdit, &QLineEdit::textChanged, this, [this]() { m_nameError->clear(); });
    m_nameError = new QLabel(this);
    m_nameError->setObjectName(QStringLiteral("fieldError"));

    m_iconEdit = new QLineEdit(dutyType.icon(), this);
    m_iconEdit->setPlaceholderText(QStringLiteral("A single emoji, e.g. \U0001F6CE️"));

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("Name"), m_nameEdit);
    form->addRow(QString(), m_nameError);
    form->addRow(QStringLiteral("Icon"), m_iconEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &DutyTypeEditDialog::saveClicked);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = contentLayout();
    layout->addWidget(heading);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

void DutyTypeEditDialog::saveClicked()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        m_nameError->setText(QStringLiteral("Name is required."));
        return;
    }
    accept();
}

DutyType DutyTypeEditDialog::dutyType() const
{
    const QString icon = m_iconEdit->text().trimmed();
    return DutyType(m_id, m_nameEdit->text().trimmed(), icon.isEmpty() ? QStringLiteral("\U0001F4CB") : icon);
}

#include "TeamEditDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QVBoxLayout>

TeamEditDialog::TeamEditDialog(const Team &team, QWidget *parent)
    : FramelessDialog(parent)
    , m_id(team.id())
{
    const QString title = team.id() < 0 ? tr("Add Team") : tr("Edit Team");
    setWindowTitle(title);
    auto *heading = new QLabel(title, this);
    heading->setObjectName(QStringLiteral("pageTitle"));

    m_nameEdit = new QLineEdit(team.name(), this);
    m_nameEdit->setPlaceholderText(tr("e.g. Choir"));
    connect(m_nameEdit, &QLineEdit::textChanged, this, [this]() { m_nameError->clear(); });
    m_nameError = new QLabel(this);
    m_nameError->setObjectName(QStringLiteral("fieldError"));

    m_descriptionEdit = new QPlainTextEdit(team.description(), this);
    m_descriptionEdit->setFixedHeight(60);
    m_descriptionEdit->setPlaceholderText(tr("Optional notes about what this team does"));

    auto *form = new QFormLayout;
    form->addRow(tr("Name"), m_nameEdit);
    form->addRow(QString(), m_nameError);
    form->addRow(tr("Description"), m_descriptionEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &TeamEditDialog::saveClicked);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = contentLayout();
    layout->addWidget(heading);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

void TeamEditDialog::saveClicked()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        m_nameError->setText(tr("Name is required."));
        return;
    }
    accept();
}

Team TeamEditDialog::team() const
{
    return Team(m_id, m_nameEdit->text().trimmed(), m_descriptionEdit->toPlainText().trimmed());
}

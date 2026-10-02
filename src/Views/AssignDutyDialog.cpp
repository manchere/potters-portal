#include "AssignDutyDialog.h"

#include <QLocale>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

#include "Controllers/DutyTypeController.h"
#include "Models/DutyType.h"
#include "SuggestLineEdit.h"

AssignDutyDialog::AssignDutyDialog(
    const Duty &duty,
    const QDate &serviceDate,
    const QVector<User> &members,
    DutyTypeController *dutyTypeController,
    QWidget *parent)
    : FramelessDialog(parent)
    , m_id(duty.id())
    , m_serviceDate(serviceDate)
{
    const QString title = duty.id() < 0 ? tr("Assign Duty") : tr("Edit Duty");
    setWindowTitle(title);
    auto *heading = new QLabel(title, this);
    heading->setObjectName(QStringLiteral("pageTitle"));

    auto *dateLabel = new QLabel(
        tr("Sunday: %1").arg(QLocale().toString(serviceDate, QStringLiteral("yyyy-MM-dd"))), this);
    dateLabel->setObjectName(QStringLiteral("pageSubtitle"));

    QList<QPair<int, QString>> dutyTypes;
    for (const DutyType &dutyType : dutyTypeController->allDutyTypes()) {
        dutyTypes.append({dutyType.id(), dutyType.iconAndName()});
    }
    m_dutyTypeEdit = new SuggestLineEdit(this);
    m_dutyTypeEdit->setItems(dutyTypes);
    m_dutyTypeEdit->setPlaceholderText(tr("Type a duty, e.g. Ushering"));
    m_dutyTypeEdit->setCurrentId(duty.dutyTypeId());

    // Left empty means nobody (the old "None" choice).
    QList<QPair<int, QString>> memberItems;
    for (const User &user : members) {
        memberItems.append({user.id(), user.name()});
    }
    m_memberEdit = new SuggestLineEdit(this);
    m_supportMemberEdit = new SuggestLineEdit(this);
    for (SuggestLineEdit *edit : {m_memberEdit, m_supportMemberEdit}) {
        edit->setItems(memberItems);
        edit->setPlaceholderText(tr("Type a name, or leave empty for nobody"));
    }
    m_memberEdit->setCurrentId(duty.memberId());
    m_supportMemberEdit->setCurrentId(duty.supportMemberId());

    m_notesEdit = new QPlainTextEdit(duty.notes(), this);
    m_notesEdit->setFixedHeight(60);
    m_notesEdit->setPlaceholderText(tr("Optional notes"));

    auto *form = new QFormLayout;
    form->addRow(tr("Duty"), m_dutyTypeEdit);
    form->addRow(tr("Member"), m_memberEdit);
    form->addRow(tr("Support member"), m_supportMemberEdit);
    form->addRow(tr("Notes"), m_notesEdit);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName(QStringLiteral("fieldError"));
    m_errorLabel->setWordWrap(true);
    for (SuggestLineEdit *edit : {m_dutyTypeEdit, m_memberEdit, m_supportMemberEdit}) {
        connect(edit, &QLineEdit::textChanged, m_errorLabel, &QLabel::clear);
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &AssignDutyDialog::saveClicked);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = contentLayout();
    layout->addWidget(heading);
    layout->addWidget(dateLabel);
    layout->addLayout(form);
    layout->addWidget(m_errorLabel);
    layout->addWidget(buttons);
}

void AssignDutyDialog::saveClicked()
{
    if (m_dutyTypeEdit->currentId() < 0) {
        m_errorLabel->setText(m_dutyTypeEdit->text().trimmed().isEmpty()
            ? tr("Pick a duty.")
            : tr("\"%1\" isn't a duty -- pick one from the suggestions.").arg(m_dutyTypeEdit->text().trimmed()));
        m_dutyTypeEdit->setFocus();
        return;
    }
    for (SuggestLineEdit *edit : {m_memberEdit, m_supportMemberEdit}) {
        if (edit->hasUnknownText()) {
            m_errorLabel->setText(tr("No member is called \"%1\" -- pick one from the suggestions, "
                                                 "or leave it empty.").arg(edit->text().trimmed()));
            edit->setFocus();
            return;
        }
    }
    accept();
}

Duty AssignDutyDialog::duty() const
{
    return Duty(
        m_id,
        m_dutyTypeEdit->currentId(),
        m_serviceDate,
        m_memberEdit->currentId(),
        m_supportMemberEdit->currentId(),
        m_notesEdit->toPlainText().trimmed());
}

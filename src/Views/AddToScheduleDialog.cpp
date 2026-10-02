#include "AddToScheduleDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "Controllers/DutyTypeController.h"
#include "MemberPickerField.h"
#include "Models/DutyType.h"
#include "SuggestLineEdit.h"

AddToScheduleDialog::AddToScheduleDialog(
    const QDate &serviceDate,
    const QVector<User> &members,
    DutyTypeController *dutyTypeController,
    QWidget *parent,
    const QVector<Duty> &existingDuties)
    : FramelessDialog(parent)
    , m_serviceDate(serviceDate)
{
    const bool isEdit = !existingDuties.isEmpty();
    const QString title = isEdit ? QStringLiteral("Edit Member on Schedule") : QStringLiteral("Add Member to Schedule");
    setWindowTitle(title);
    auto *heading = new QLabel(title, this);
    heading->setObjectName(QStringLiteral("pageTitle"));
    auto *dateLabel = new QLabel(serviceDate.toString(QStringLiteral("dddd d MMMM yyyy")), this);
    dateLabel->setObjectName(QStringLiteral("pageSubtitle"));

    QList<QPair<int, QString>> memberItems;
    for (const User &user : members) {
        memberItems.append({user.id(), user.name()});
    }
    m_memberEdit = new SuggestLineEdit(this);
    m_memberEdit->setItems(memberItems);
    m_memberEdit->setPlaceholderText(QStringLiteral("Type the member's name"));

    QList<QPair<int, QString>> dutyItems;
    for (const DutyType &dutyType : dutyTypeController->allDutyTypes()) {
        dutyItems.append({dutyType.id(), dutyType.iconAndName()});
    }
    m_dutyPicker = new MemberPickerField(this);
    m_dutyPicker->setPlaceholders(QStringLiteral("Type a duty to add it"), QStringLiteral("Add another duty..."));
    m_dutyPicker->setMembers(dutyItems);
    m_dutyPicker->setMinimumWidth(340);

    m_backupEdit = new SuggestLineEdit(this);
    m_backupEdit->setItems(memberItems);
    m_backupEdit->setPlaceholderText(QStringLiteral("Covers if they're not around (optional)"));

    m_notesEdit = new QPlainTextEdit(this);
    m_notesEdit->setFixedHeight(60);
    m_notesEdit->setPlaceholderText(QStringLiteral("Optional notes"));

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("Member"), m_memberEdit);
    form->addRow(QStringLiteral("Duties"), m_dutyPicker);
    form->addRow(QStringLiteral("Backup"), m_backupEdit);
    form->addRow(QStringLiteral("Notes"), m_notesEdit);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName(QStringLiteral("fieldError"));
    m_errorLabel->setWordWrap(true);
    connect(m_memberEdit, &QLineEdit::textChanged, m_errorLabel, &QLabel::clear);
    connect(m_backupEdit, &QLineEdit::textChanged, m_errorLabel, &QLabel::clear);
    connect(m_dutyPicker, &MemberPickerField::selectionChanged, m_errorLabel, &QLabel::clear);

    if (isEdit) {
        // Same person throughout -- to put someone else on, use Add Member.
        const Duty &first = existingDuties.first();
        m_memberEdit->setCurrentId(first.memberId());
        m_memberEdit->setReadOnly(true);
        QVector<int> dutyTypeIds;
        for (const Duty &duty : existingDuties) {
            dutyTypeIds.append(duty.dutyTypeId());
            m_mixedBackups = m_mixedBackups || duty.supportMemberId() != first.supportMemberId();
            m_mixedNotes = m_mixedNotes || duty.notes() != first.notes();
        }
        m_dutyPicker->setSelectedIds(dutyTypeIds);
        if (m_mixedBackups) {
            m_backupEdit->setPlaceholderText(QStringLiteral("Differs per duty -- leave empty to keep each one's backup"));
        } else {
            m_backupEdit->setCurrentId(first.supportMemberId());
        }
        if (m_mixedNotes) {
            m_notesEdit->setPlaceholderText(QStringLiteral("Differs per duty -- leave empty to keep each one's notes"));
        } else {
            m_notesEdit->setPlainText(first.notes());
        }
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Save)->setText(isEdit ? QStringLiteral("Save Changes") : QStringLiteral("Add to Schedule"));
    connect(buttons, &QDialogButtonBox::accepted, this, &AddToScheduleDialog::saveClicked);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = contentLayout();
    layout->addWidget(heading);
    layout->addWidget(dateLabel);
    layout->addLayout(form);
    layout->addWidget(m_errorLabel);
    layout->addWidget(buttons);
}

void AddToScheduleDialog::saveClicked()
{
    const int memberId = m_memberEdit->currentId();
    if (memberId < 0) {
        m_errorLabel->setText(m_memberEdit->text().trimmed().isEmpty()
            ? QStringLiteral("Pick the member to add.")
            : QStringLiteral("No member is called \"%1\" -- pick one from the suggestions.").arg(m_memberEdit->text().trimmed()));
        m_memberEdit->setFocus();
        return;
    }
    if (m_dutyPicker->selectedIds().isEmpty()) {
        m_errorLabel->setText(QStringLiteral("Add at least one duty for them."));
        return;
    }
    if (m_backupEdit->hasUnknownText()) {
        m_errorLabel->setText(QStringLiteral("No member is called \"%1\" -- pick a backup from the suggestions, "
                                             "or leave it empty.").arg(m_backupEdit->text().trimmed()));
        m_backupEdit->setFocus();
        return;
    }
    if (m_backupEdit->currentId() == memberId) {
        m_errorLabel->setText(QStringLiteral("The backup has to be someone else."));
        m_backupEdit->setFocus();
        return;
    }
    accept();
}

bool AddToScheduleDialog::keepsEachBackup() const
{
    return m_mixedBackups && m_backupEdit->text().trimmed().isEmpty();
}

bool AddToScheduleDialog::keepsEachNotes() const
{
    return m_mixedNotes && m_notesEdit->toPlainText().trimmed().isEmpty();
}

QVector<Duty> AddToScheduleDialog::duties() const
{
    QVector<Duty> result;
    const QString notes = m_notesEdit->toPlainText().trimmed();
    for (int dutyTypeId : m_dutyPicker->selectedIds()) {
        result.append(Duty(-1, dutyTypeId, m_serviceDate, m_memberEdit->currentId(), m_backupEdit->currentId(), notes));
    }
    return result;
}

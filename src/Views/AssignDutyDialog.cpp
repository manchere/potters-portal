#include "AssignDutyDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

#include "Controllers/DutyTypeController.h"
#include "Models/DutyType.h"

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
    const QString title = duty.id() < 0 ? QStringLiteral("Assign Duty") : QStringLiteral("Edit Duty");
    setWindowTitle(title);
    auto *heading = new QLabel(title, this);
    heading->setObjectName(QStringLiteral("pageTitle"));

    auto *dateLabel = new QLabel(
        QStringLiteral("Sunday: %1").arg(serviceDate.toString(QStringLiteral("yyyy-MM-dd"))), this);
    dateLabel->setObjectName(QStringLiteral("pageSubtitle"));

    m_dutyTypeCombo = new QComboBox(this);
    for (const DutyType &dutyType : dutyTypeController->allDutyTypes()) {
        m_dutyTypeCombo->addItem(dutyType.iconAndName(), dutyType.id());
    }
    const int dutyTypeIdx = m_dutyTypeCombo->findData(duty.dutyTypeId());
    m_dutyTypeCombo->setCurrentIndex(dutyTypeIdx >= 0 ? dutyTypeIdx : 0);

    m_memberCombo = new QComboBox(this);
    m_supportMemberCombo = new QComboBox(this);
    for (QComboBox *combo : {m_memberCombo, m_supportMemberCombo}) {
        combo->addItem(QStringLiteral("None"), -1);
    }
    for (const User &user : members) {
        m_memberCombo->addItem(user.name(), user.id());
        m_supportMemberCombo->addItem(user.name(), user.id());
    }
    const int memberIdx = m_memberCombo->findData(duty.memberId());
    m_memberCombo->setCurrentIndex(memberIdx >= 0 ? memberIdx : 0);
    const int supportIdx = m_supportMemberCombo->findData(duty.supportMemberId());
    m_supportMemberCombo->setCurrentIndex(supportIdx >= 0 ? supportIdx : 0);

    m_notesEdit = new QPlainTextEdit(duty.notes(), this);
    m_notesEdit->setFixedHeight(60);
    m_notesEdit->setPlaceholderText(QStringLiteral("Optional notes"));

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("Duty"), m_dutyTypeCombo);
    form->addRow(QStringLiteral("Member"), m_memberCombo);
    form->addRow(QStringLiteral("Support member"), m_supportMemberCombo);
    form->addRow(QStringLiteral("Notes"), m_notesEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = contentLayout();
    layout->addWidget(heading);
    layout->addWidget(dateLabel);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

Duty AssignDutyDialog::duty() const
{
    return Duty(
        m_id,
        m_dutyTypeCombo->currentData().toInt(),
        m_serviceDate,
        m_memberCombo->currentData().toInt(),
        m_supportMemberCombo->currentData().toInt(),
        m_notesEdit->toPlainText().trimmed());
}

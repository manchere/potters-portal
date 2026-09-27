#include "AssignRoleDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

#include "Controllers/RoleTypeController.h"
#include "Models/RoleType.h"

AssignRoleDialog::AssignRoleDialog(
    const Assignment &assignment,
    const QDate &serviceDate,
    const QVector<User> &members,
    RoleTypeController *roleTypeController,
    QWidget *parent)
    : FramelessDialog(parent)
    , m_id(assignment.id())
    , m_serviceDate(serviceDate)
{
    const QString title = assignment.id() < 0 ? QStringLiteral("Assign Role") : QStringLiteral("Edit Assignment");
    setWindowTitle(title);
    auto *heading = new QLabel(title, this);
    heading->setObjectName(QStringLiteral("pageTitle"));

    auto *dateLabel = new QLabel(
        QStringLiteral("Sunday: %1").arg(serviceDate.toString(QStringLiteral("yyyy-MM-dd"))), this);
    dateLabel->setObjectName(QStringLiteral("pageSubtitle"));

    m_roleCombo = new QComboBox(this);
    for (const RoleType &roleType : roleTypeController->allRoleTypes()) {
        m_roleCombo->addItem(roleType.iconAndName(), roleType.id());
    }
    const int roleIdx = m_roleCombo->findData(assignment.roleId());
    m_roleCombo->setCurrentIndex(roleIdx >= 0 ? roleIdx : 0);

    m_memberCombo = new QComboBox(this);
    m_supportMemberCombo = new QComboBox(this);
    for (QComboBox *combo : {m_memberCombo, m_supportMemberCombo}) {
        combo->addItem(QStringLiteral("None"), -1);
    }
    for (const User &user : members) {
        m_memberCombo->addItem(user.name(), user.id());
        m_supportMemberCombo->addItem(user.name(), user.id());
    }
    const int memberIdx = m_memberCombo->findData(assignment.memberId());
    m_memberCombo->setCurrentIndex(memberIdx >= 0 ? memberIdx : 0);
    const int supportIdx = m_supportMemberCombo->findData(assignment.supportMemberId());
    m_supportMemberCombo->setCurrentIndex(supportIdx >= 0 ? supportIdx : 0);

    m_notesEdit = new QPlainTextEdit(assignment.notes(), this);
    m_notesEdit->setFixedHeight(60);
    m_notesEdit->setPlaceholderText(QStringLiteral("Optional notes"));

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("Role"), m_roleCombo);
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

Assignment AssignRoleDialog::assignment() const
{
    return Assignment(
        m_id,
        m_roleCombo->currentData().toInt(),
        m_serviceDate,
        m_memberCombo->currentData().toInt(),
        m_supportMemberCombo->currentData().toInt(),
        m_notesEdit->toPlainText().trimmed());
}

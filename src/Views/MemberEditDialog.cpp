#include "MemberEditDialog.h"

#include <algorithm>

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "Auth/PasswordAuth.h"
#include "Controllers/TeamController.h"
#include "Controllers/UserController.h"
#include "MemberBadge.h"
#include "MemberColorPicker.h"
#include "Models/MemberColors.h"

MemberEditDialog::MemberEditDialog(
    const User &user,
    UserController *userController,
    TeamController *teamController,
    QWidget *parent)
    : FramelessDialog(parent)
    , m_existingUser(user)
    , m_userController(userController)
{
    const bool isNew = user.id() < 0;
    const QString title = isNew ? tr("Add Member") : tr("Edit Member");
    setWindowTitle(title);
    auto *heading = new QLabel(title, this);
    heading->setObjectName(QStringLiteral("pageTitle"));

    // The Member's circle as it will appear elsewhere, above the form.
    m_badgePreview = MemberBadge::make(user.name(), user.color(), 72, this);
    auto *badgeRow = new QVBoxLayout;
    badgeRow->addWidget(m_badgePreview, 0, Qt::AlignHCenter);

    m_colorPicker = new MemberColorPicker(this);
    m_colorPicker->setColor(isNew ? MemberColors::defaultColor() : user.color());
    connect(m_colorPicker, &MemberColorPicker::colorChanged, this, &MemberEditDialog::updateBadgePreview);

    auto errorLabel = [this]() {
        auto *label = new QLabel(this);
        label->setObjectName(QStringLiteral("fieldError"));
        label->setWordWrap(true);
        return label;
    };

    m_nameEdit = new QLineEdit(user.name(), this);
    connect(m_nameEdit, &QLineEdit::textChanged, this, &MemberEditDialog::updateBadgePreview);
    connect(m_nameEdit, &QLineEdit::textChanged, this, [this]() { m_nameError->clear(); });
    m_nameError = errorLabel();

    m_teamCombo = new QComboBox(this);
    m_teamCombo->addItem(tr("No team"), -1);
    for (const Team &team : teamController->allTeams()) {
        m_teamCombo->addItem(team.name(), team.id());
    }
    m_teamCombo->setCurrentIndex(std::max(0, m_teamCombo->findData(user.teamId())));

    m_phoneEdit = new QLineEdit(user.phone(), this);
    m_phoneEdit->setPlaceholderText(tr("What they sign in with"));
    m_phoneEdit->setInputMethodHints(Qt::ImhDialableCharactersOnly);
    connect(m_phoneEdit, &QLineEdit::textChanged, this, [this]() { m_phoneError->clear(); });
    m_phoneError = errorLabel();

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(isNew
        ? tr("Min. 8 characters -- share this with the member")
        : tr("Leave blank to keep the current password"));
    connect(m_passwordEdit, &QLineEdit::textChanged, this, [this]() { m_passwordError->clear(); });
    m_passwordError = errorLabel();

    m_showPasswordCheck = new QCheckBox(tr("Show password"), this);
    connect(m_showPasswordCheck, &QCheckBox::toggled, this, &MemberEditDialog::toggleShowPassword);

    auto *form = new QFormLayout;
    form->addRow(tr("Name"), m_nameEdit);
    form->addRow(QString(), m_nameError);
    form->addRow(tr("Color"), m_colorPicker);
    form->addRow(tr("Team"), m_teamCombo);
    form->addRow(tr("Phone number"), m_phoneEdit);
    form->addRow(QString(), m_phoneError);
    form->addRow(tr("Password"), m_passwordEdit);
    form->addRow(QString(), m_passwordError);
    form->addRow(QString(), m_showPasswordCheck);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Save)->setText(isNew ? tr("Add Member") : tr("Save Changes"));
    connect(buttons, &QDialogButtonBox::accepted, this, &MemberEditDialog::saveClicked);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = contentLayout();
    layout->addWidget(heading);
    layout->addLayout(badgeRow);
    layout->addLayout(form);
    layout->addWidget(buttons);

    updateBadgePreview();
}

void MemberEditDialog::toggleShowPassword(bool show)
{
    m_passwordEdit->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
}

void MemberEditDialog::updateBadgePreview()
{
    const QString name = m_nameEdit->text().trimmed();
    MemberBadge::update(m_badgePreview, name.isEmpty() ? QStringLiteral("?") : name, m_colorPicker->color());
}

bool MemberEditDialog::validate()
{
    const bool isNew = m_existingUser.id() < 0;
    bool valid = true;
    if (m_nameEdit->text().trimmed().isEmpty()) {
        m_nameError->setText(tr("Name is required."));
        valid = false;
    }
    const QString phone = m_phoneEdit->text().trimmed();
    if (phone.isEmpty()) {
        m_phoneError->setText(tr("Phone number is required."));
        valid = false;
    } else if (!UserController::isValidPhone(phone)) {
        m_phoneError->setText(tr("Enter a valid phone number."));
        valid = false;
    }
    const QString password = m_passwordEdit->text();
    if (isNew && password.length() < 8) {
        m_passwordError->setText(tr("Password must be at least 8 characters."));
        valid = false;
    } else if (!isNew && !password.isEmpty() && password.length() < 8) {
        m_passwordError->setText(tr("Password must be at least 8 characters (or leave it blank)."));
        valid = false;
    }
    return valid;
}

void MemberEditDialog::saveClicked()
{
    if (!validate()) {
        return;
    }

    User user = m_existingUser;
    user.setName(m_nameEdit->text().trimmed());
    user.setPhone(m_phoneEdit->text().trimmed());
    user.setColor(m_colorPicker->color());
    user.setTeamId(m_teamCombo->currentData().toInt());

    const QString password = m_passwordEdit->text();
    if (!password.isEmpty()) {
        const QString salt = PasswordAuth::generateSalt();
        user.setPasswordSalt(salt);
        user.setPasswordHash(PasswordAuth::hashPassword(password, salt));
    }
    // If password is empty and this is an edit, user already carries the
    // existing passwordHash/passwordSalt from m_existingUser unchanged.

    const bool ok = m_existingUser.id() < 0 ? m_userController->addUser(user) : m_userController->updateUser(user);
    if (!ok) {
        m_phoneError->setText(m_userController->lastError());
        return;
    }
    accept();
}

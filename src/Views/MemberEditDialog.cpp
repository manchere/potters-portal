#include "MemberEditDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QVBoxLayout>

#include "Auth/PasswordAuth.h"
#include "AvatarLoader.h"
#include "Controllers/UserController.h"

namespace
{
    // Deliberately loose -- just enough to catch "forgot the @" or "forgot
    // the domain" typos, not a full RFC 5322 validator.
    bool looksLikeEmail(const QString &value)
    {
        static const QRegularExpression pattern(QStringLiteral("^[^@\\s]+@[^@\\s]+\\.[^@\\s]+$"));
        return pattern.match(value).hasMatch();
    }
}

MemberEditDialog::MemberEditDialog(
    const User &user,
    UserController *userController,
    QNetworkAccessManager *networkManager,
    QWidget *parent)
    : FramelessDialog(parent)
    , m_existingUser(user)
    , m_userController(userController)
    , m_networkManager(networkManager)
{
    const bool isNew = user.id() < 0;
    const QString title = isNew ? QStringLiteral("Add Member") : QStringLiteral("Edit Member");
    setWindowTitle(title);
    auto *heading = new QLabel(title, this);
    heading->setObjectName(QStringLiteral("pageTitle"));

    m_avatarPreview = new QLabel(this);
    m_avatarPreview->setAlignment(Qt::AlignCenter);
    auto *avatarRow = new QVBoxLayout;
    avatarRow->addWidget(m_avatarPreview, 0, Qt::AlignHCenter);

    auto errorLabel = [this]() {
        auto *label = new QLabel(this);
        label->setObjectName(QStringLiteral("fieldError"));
        label->setWordWrap(true);
        return label;
    };

    m_nameEdit = new QLineEdit(user.name(), this);
    m_nameEdit->setPlaceholderText(QStringLiteral("e.g. Grace Adeyemi"));
    connect(m_nameEdit, &QLineEdit::textChanged, this, &MemberEditDialog::updateAvatarPreview);
    connect(m_nameEdit, &QLineEdit::textChanged, this, [this]() { m_nameError->clear(); });
    m_nameError = errorLabel();

    m_emailEdit = new QLineEdit(user.email(), this);
    m_emailEdit->setPlaceholderText(QStringLiteral("member@example.com"));
    connect(m_emailEdit, &QLineEdit::textChanged, this, [this]() { m_emailError->clear(); });
    m_emailError = errorLabel();

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(isNew
        ? QStringLiteral("Min. 8 characters -- share this with the member")
        : QStringLiteral("Leave blank to keep the current password"));
    connect(m_passwordEdit, &QLineEdit::textChanged, this, [this]() { m_passwordError->clear(); });
    m_passwordError = errorLabel();

    m_showPasswordCheck = new QCheckBox(QStringLiteral("Show password"), this);
    connect(m_showPasswordCheck, &QCheckBox::toggled, this, &MemberEditDialog::toggleShowPassword);

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("Name"), m_nameEdit);
    form->addRow(QString(), m_nameError);
    form->addRow(QStringLiteral("Email"), m_emailEdit);
    form->addRow(QString(), m_emailError);
    form->addRow(QStringLiteral("Password"), m_passwordEdit);
    form->addRow(QString(), m_passwordError);
    form->addRow(QString(), m_showPasswordCheck);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Save)->setText(isNew ? QStringLiteral("Add Member") : QStringLiteral("Save Changes"));
    connect(buttons, &QDialogButtonBox::accepted, this, &MemberEditDialog::saveClicked);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = contentLayout();
    layout->addWidget(heading);
    layout->addLayout(avatarRow);
    layout->addLayout(form);
    layout->addWidget(buttons);

    updateAvatarPreview();
}

void MemberEditDialog::toggleShowPassword(bool show)
{
    m_passwordEdit->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
}

void MemberEditDialog::updateAvatarPreview()
{
    const QString seed = m_nameEdit->text().trimmed().isEmpty() ? QStringLiteral("member") : m_nameEdit->text().trimmed();
    if (m_networkManager) {
        AvatarLoader::loadInto(*m_networkManager, seed, m_avatarPreview, 72);
    }
}

bool MemberEditDialog::validate()
{
    const bool isNew = m_existingUser.id() < 0;
    bool valid = true;
    if (m_nameEdit->text().trimmed().isEmpty()) {
        m_nameError->setText(QStringLiteral("Name is required."));
        valid = false;
    }
    const QString email = m_emailEdit->text().trimmed();
    if (email.isEmpty()) {
        m_emailError->setText(QStringLiteral("Email is required."));
        valid = false;
    } else if (!looksLikeEmail(email)) {
        m_emailError->setText(QStringLiteral("Enter a valid email address."));
        valid = false;
    }
    const QString password = m_passwordEdit->text();
    if (isNew && password.length() < 8) {
        m_passwordError->setText(QStringLiteral("Password must be at least 8 characters."));
        valid = false;
    } else if (!isNew && !password.isEmpty() && password.length() < 8) {
        m_passwordError->setText(QStringLiteral("Password must be at least 8 characters (or leave it blank)."));
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
    user.setEmail(m_emailEdit->text().trimmed());
    user.setAvatarSeed(m_nameEdit->text().trimmed());

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
        m_emailError->setText(m_userController->lastError());
        return;
    }
    accept();
}

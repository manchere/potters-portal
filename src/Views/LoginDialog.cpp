#include "LoginDialog.h"

#include <QCheckBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#include "AdminContact.h"
#include "ClickableLabel.h"
#include "Controllers/UserController.h"

LoginDialog::LoginDialog(UserController *userController, QWidget *parent)
    : FramelessDialog(parent)
    , m_userController(userController)
{
    setWindowTitle(tr("Sign In"));

    auto *heading = new QLabel(tr("Sign In"), this);
    heading->setObjectName(QStringLiteral("pageTitle"));

    m_phoneEdit = new QLineEdit(this);
    m_phoneEdit->setPlaceholderText(tr("Your phone number"));
    m_phoneEdit->setInputMethodHints(Qt::ImhDialableCharactersOnly);
    m_phoneEdit->setFocus();
    connect(m_phoneEdit, &QLineEdit::textChanged, this, [this]() { m_errorLabel->clear(); });

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    connect(m_passwordEdit, &QLineEdit::textChanged, this, [this]() { m_errorLabel->clear(); });

    m_showPasswordCheck = new QCheckBox(tr("Show password"), this);
    connect(m_showPasswordCheck, &QCheckBox::toggled, this, &LoginDialog::toggleShowPassword);

    auto *form = new QFormLayout;
    form->addRow(tr("Phone number"), m_phoneEdit);
    form->addRow(tr("Password"), m_passwordEdit);
    form->addRow(QString(), m_showPasswordCheck);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName(QStringLiteral("fieldError"));
    m_errorLabel->setWordWrap(true);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Sign In"));
    connect(buttons, &QDialogButtonBox::accepted, this, &LoginDialog::attemptLogin);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Opens a new email to the admin contact in the default mail app; if
    // there isn't one, the address is shown instead so it can be copied.
    // A ClickableLabel rather than a QLabel link: the dialog drags itself on
    // any press a child doesn't take, and a link label doesn't take it.
    auto *forgotLink = new ClickableLabel(this);
    forgotLink->setText(tr("<a href='#'>Forgot password?</a>"));
    forgotLink->setTextInteractionFlags(Qt::NoTextInteraction);
    forgotLink->setToolTip(tr("Email %1 to reset your password").arg(kAdminContactEmail));
    forgotLink->setAlignment(Qt::AlignRight);
    connect(forgotLink, &ClickableLabel::clicked, this, [this]() {
        QUrl mail(QStringLiteral("mailto:") + kAdminContactEmail);
        mail.setQuery(QStringLiteral("subject=") + tr("Potters Portal password reset"));
        if (!QDesktopServices::openUrl(mail)) {
            m_errorLabel->setText(tr("Email %1 to reset your password.").arg(kAdminContactEmail));
            m_errorLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        }
    });

    auto *layout = contentLayout();
    layout->addWidget(heading);
    layout->addLayout(form);
    layout->addWidget(m_errorLabel);
    layout->addWidget(buttons);
    layout->addWidget(forgotLink);
    // Roomier sides than the other dialogs: it's small, and the fields
    // looked cramped against the edges.
    layout->setContentsMargins(36, 24, 36, 24);
    setMinimumWidth(420);
}

void LoginDialog::toggleShowPassword(bool show)
{
    m_passwordEdit->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
}

void LoginDialog::attemptLogin()
{
    User user;
    if (!m_userController->verifyPassword(m_phoneEdit->text().trimmed(), m_passwordEdit->text(), user)) {
        m_errorLabel->setText(tr("Incorrect phone number or password."));
        return;
    }
    m_user = user;
    accept();
}

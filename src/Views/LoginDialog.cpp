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
    setWindowTitle(QStringLiteral("Admin Login"));

    auto *heading = new QLabel(QStringLiteral("Admin Login"), this);
    heading->setObjectName(QStringLiteral("pageTitle"));

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setFocus();
    connect(m_passwordEdit, &QLineEdit::textChanged, this, [this]() { m_errorLabel->clear(); });

    m_showPasswordCheck = new QCheckBox(QStringLiteral("Show password"), this);
    connect(m_showPasswordCheck, &QCheckBox::toggled, this, &LoginDialog::toggleShowPassword);

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("Password"), m_passwordEdit);
    form->addRow(QString(), m_showPasswordCheck);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName(QStringLiteral("fieldError"));
    m_errorLabel->setWordWrap(true);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Log In"));
    connect(buttons, &QDialogButtonBox::accepted, this, &LoginDialog::attemptLogin);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Opens a new email to the admin contact in the default mail app; if
    // there isn't one, the address is shown instead so it can be copied.
    // A ClickableLabel rather than a QLabel link: the dialog drags itself on
    // any press a child doesn't take, and a link label doesn't take it.
    auto *forgotLink = new ClickableLabel(this);
    forgotLink->setText(QStringLiteral("<a href='#'>Forgot password?</a>"));
    forgotLink->setTextInteractionFlags(Qt::NoTextInteraction);
    forgotLink->setToolTip(QStringLiteral("Email %1 to reset the admin password").arg(kAdminContactEmail));
    forgotLink->setAlignment(Qt::AlignRight);
    connect(forgotLink, &ClickableLabel::clicked, this, [this]() {
        QUrl mail(QStringLiteral("mailto:") + kAdminContactEmail);
        mail.setQuery(QStringLiteral("subject=Potter's Portal admin password reset"));
        if (!QDesktopServices::openUrl(mail)) {
            m_errorLabel->setText(QStringLiteral("Email %1 to reset the admin password.").arg(kAdminContactEmail));
            m_errorLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        }
    });

    auto *layout = contentLayout();
    layout->addWidget(heading);
    layout->addLayout(form);
    layout->addWidget(m_errorLabel);
    layout->addWidget(buttons);
    layout->addWidget(forgotLink);
}

void LoginDialog::toggleShowPassword(bool show)
{
    m_passwordEdit->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
}

void LoginDialog::attemptLogin()
{
    User user;
    if (!m_userController->verifyAdminPassword(m_passwordEdit->text(), user)) {
        m_errorLabel->setText(QStringLiteral("Incorrect password."));
        return;
    }
    m_user = user;
    accept();
}

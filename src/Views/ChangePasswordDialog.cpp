#include "ChangePasswordDialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "AdminContact.h"
#include "Controllers/UserController.h"
#include "Mail/SmtpMail.h"

namespace
{
    // Same minimum as MemberEditDialog.
    constexpr int kMinPasswordLength = 8;
}

ChangePasswordDialog::ChangePasswordDialog(const User &admin, UserController *userController, QWidget *parent)
    : FramelessDialog(parent)
    , m_admin(admin)
    , m_userController(userController)
{
    setWindowTitle(QStringLiteral("Change Password"));

    auto *heading = new QLabel(QStringLiteral("Change Password"), this);
    heading->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(
        QStringLiteral("For %1. The new password will be emailed to you and to %2.")
            .arg(admin.name(), kAdminContactEmail),
        this);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));
    subtitle->setWordWrap(true);

    m_newPasswordEdit = new QLineEdit(this);
    m_newPasswordEdit->setEchoMode(QLineEdit::Password);
    m_newPasswordEdit->setPlaceholderText(QStringLiteral("At least %1 characters").arg(kMinPasswordLength));
    m_newPasswordEdit->setFocus();
    m_confirmEdit = new QLineEdit(this);
    m_confirmEdit->setEchoMode(QLineEdit::Password);
    for (QLineEdit *edit : {m_newPasswordEdit, m_confirmEdit}) {
        connect(edit, &QLineEdit::textChanged, this, [this]() { m_errorLabel->clear(); });
    }

    m_showPasswordCheck = new QCheckBox(QStringLiteral("Show password"), this);
    connect(m_showPasswordCheck, &QCheckBox::toggled, this, &ChangePasswordDialog::toggleShowPassword);

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("New password"), m_newPasswordEdit);
    form->addRow(QStringLiteral("Confirm"), m_confirmEdit);
    form->addRow(QString(), m_showPasswordCheck);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName(QStringLiteral("fieldError"));
    m_errorLabel->setWordWrap(true);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Change Password"));
    connect(m_buttons, &QDialogButtonBox::accepted, this, &ChangePasswordDialog::submit);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = contentLayout();
    layout->addWidget(heading);
    layout->addWidget(subtitle);
    layout->addLayout(form);
    layout->addWidget(m_errorLabel);
    layout->addWidget(m_buttons);
}

void ChangePasswordDialog::toggleShowPassword(bool show)
{
    const QLineEdit::EchoMode mode = show ? QLineEdit::Normal : QLineEdit::Password;
    m_newPasswordEdit->setEchoMode(mode);
    m_confirmEdit->setEchoMode(mode);
}

void ChangePasswordDialog::submit()
{
    const QString password = m_newPasswordEdit->text();
    if (password.length() < kMinPasswordLength) {
        m_errorLabel->setText(QStringLiteral("Password must be at least %1 characters.").arg(kMinPasswordLength));
        return;
    }
    if (password != m_confirmEdit->text()) {
        m_errorLabel->setText(QStringLiteral("The two passwords don't match."));
        return;
    }
    if (!SmtpMail::isConfigured()) {
        m_errorLabel->setText(QStringLiteral(
            "Email isn't set up, so the new password couldn't be sent. Set SMTP_USERNAME and "
            "SMTP_PASSWORD (a Gmail app password), restart the app, and try again. Nothing was changed."));
        return;
    }

    if (!m_userController->changePassword(m_admin.id(), password)) {
        m_errorLabel->setText(QStringLiteral("Couldn't change the password: %1").arg(m_userController->lastError()));
        return;
    }

    QStringList recipients;
    for (const QString &address : {m_admin.email().trimmed(), kAdminContactEmail}) {
        if (!address.isEmpty() && !recipients.contains(address, Qt::CaseInsensitive)) {
            recipients << address;
        }
    }
    const QString body = QStringLiteral(
        "The Potters Portal admin password for %1 (%2) was changed on %3.\n\n"
        "New password: %4\n")
        .arg(m_admin.name(), m_admin.email(),
             QDateTime::currentDateTime().toString(QStringLiteral("d MMM yyyy 'at' HH:mm")), password);

    m_buttons->setEnabled(false);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString sendError;
    const bool sent = SmtpMail::send(recipients, QStringLiteral("Potters Portal admin password changed"), body, &sendError);
    QApplication::restoreOverrideCursor();
    m_buttons->setEnabled(true);

    if (sent) {
        QMessageBox::information(this, QStringLiteral("Change Password"),
            QStringLiteral("Password changed. The new password was emailed to:\n  %1")
                .arg(recipients.join(QStringLiteral("\n  "))));
    } else {
        QMessageBox::warning(this, QStringLiteral("Change Password"),
            QStringLiteral("Password changed, but the email couldn't be sent:\n%1\n\n"
                           "Make a note of the new password now.").arg(sendError));
    }
    accept();
}

#include "LoginDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

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

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("Password"), m_passwordEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Log In"));
    connect(buttons, &QDialogButtonBox::accepted, this, &LoginDialog::attemptLogin);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(heading);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

void LoginDialog::attemptLogin()
{
    User user;
    if (!m_userController->verifyAdminPassword(m_passwordEdit->text(), user)) {
        QMessageBox::warning(this, QStringLiteral("Log In"), QStringLiteral("Incorrect password."));
        return;
    }
    m_user = user;
    accept();
}

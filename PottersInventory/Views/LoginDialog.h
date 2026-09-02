#pragma once

#include "FramelessDialog.h"

#include "Models/User.h"

class QLineEdit;
class UserController;

// Password-only admin unlock, triggered by the lock icon in TitleBar
// rather than a blocking dialog at startup -- the app is usable
// read-only without logging in at all. No email/username field: the
// password alone is checked against every Admin account
// (UserController::verifyAdminPassword).
class LoginDialog : public FramelessDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(UserController *userController, QWidget *parent = nullptr);

    // Only meaningful after exec() returns QDialog::Accepted.
    User loggedInUser() const { return m_user; }

private slots:
    void attemptLogin();

private:
    UserController *m_userController = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    User m_user;
};

#pragma once

#include "FramelessDialog.h"

#include "Models/User.h"

class QCheckBox;
class QLabel;
class QLineEdit;
class UserController;

// Sign-in for any member (email + password), opened from the lock icon in
// TitleBar rather than a blocking dialog at startup -- the app is usable
// without signing in, with whatever Settings > Access Rights allows
// everyone. Admins sign in the same way and get full access.
class LoginDialog : public FramelessDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(UserController *userController, QWidget *parent = nullptr);

    // Only meaningful after exec() returns QDialog::Accepted.
    User loggedInUser() const { return m_user; }

private slots:
    void attemptLogin();
    void toggleShowPassword(bool show);

private:
    UserController *m_userController = nullptr;
    QLineEdit *m_emailEdit = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    QCheckBox *m_showPasswordCheck = nullptr;
    QLabel *m_errorLabel = nullptr;
    User m_user;
};

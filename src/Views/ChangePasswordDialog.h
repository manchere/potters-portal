#pragma once

#include "FramelessDialog.h"

#include "Models/User.h"

class QCheckBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class UserController;

// Lets the logged-in Admin set a new password for their own account by
// typing it twice. Once saved, the new password is emailed (SmtpMail) to
// the Admin's own address and to kAdminContactEmail. Refuses to change
// anything while email isn't set up, since sending it is part of the job.
class ChangePasswordDialog : public FramelessDialog
{
    Q_OBJECT

public:
    ChangePasswordDialog(const User &admin, UserController *userController, QWidget *parent = nullptr);

private slots:
    void submit();
    void toggleShowPassword(bool show);

private:
    User m_admin;
    UserController *m_userController = nullptr;
    QLineEdit *m_newPasswordEdit = nullptr;
    QLineEdit *m_confirmEdit = nullptr;
    QCheckBox *m_showPasswordCheck = nullptr;
    QLabel *m_errorLabel = nullptr;
    QDialogButtonBox *m_buttons = nullptr;
};

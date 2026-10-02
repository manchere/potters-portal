#pragma once

#include "FramelessDialog.h"

#include "Models/User.h"

class QCheckBox;
class QLabel;
class QLineEdit;
class QNetworkAccessManager;
class UserController;

// Add or edit a Member profile (SCHEDULING_FUNCTIONAL_REQUIREMENTS.md
// FR-1.1/FR-1.3): name + email + password, mirroring the mobile app's
// registration screen (name/email/password; avatar generated from the
// name, see FR-1.2) since it's the same account, just admin-initiated.
// Pass a default-constructed User() to create a new Member (from either
// the Schedule tab's "+ Add Member" or the Taxonomy tab's "+ Add Member"), or
// an existing one to edit (Taxonomy tab, double-click) -- password is
// required when adding, optional when editing (blank keeps the current
// one; see UserController::updateUser).
class MemberEditDialog : public FramelessDialog
{
    Q_OBJECT

public:
    MemberEditDialog(
        const User &user,
        UserController *userController,
        QNetworkAccessManager *networkManager,
        QWidget *parent = nullptr);

private slots:
    void saveClicked();
    void updateAvatarPreview();
    void toggleShowPassword(bool show);

private:
    bool validate();

    User m_existingUser;
    UserController *m_userController = nullptr;
    QNetworkAccessManager *m_networkManager = nullptr;

    QLabel *m_avatarPreview = nullptr;
    QLineEdit *m_nameEdit = nullptr;
    QLabel *m_nameError = nullptr;
    QLineEdit *m_emailEdit = nullptr;
    QLabel *m_emailError = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    QLabel *m_passwordError = nullptr;
    QCheckBox *m_showPasswordCheck = nullptr;
};

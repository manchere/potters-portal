#pragma once

#include "FramelessDialog.h"

#include "Models/User.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class MemberColorPicker;
class TeamController;
class UserController;

// Add or edit a Member profile (SCHEDULING_FUNCTIONAL_REQUIREMENTS.md
// FR-1.1/FR-1.3): name + phone + password, mirroring the mobile app's
// registration screen (name/phone/password + a profile color shown
// behind the Member's initials) since it's the same account, just
// admin-initiated. The Team dropdown puts the Member on one of the teams
// made on the Taxonomy tab, or on none.
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
        TeamController *teamController,
        QWidget *parent = nullptr);

private slots:
    void saveClicked();
    void updateBadgePreview();
    void toggleShowPassword(bool show);

private:
    bool validate();

    User m_existingUser;
    UserController *m_userController = nullptr;

    QLabel *m_badgePreview = nullptr;
    MemberColorPicker *m_colorPicker = nullptr;
    QLineEdit *m_nameEdit = nullptr;
    QLabel *m_nameError = nullptr;
    QComboBox *m_teamCombo = nullptr;
    QLineEdit *m_phoneEdit = nullptr;
    QLabel *m_phoneError = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    QLabel *m_passwordError = nullptr;
    QCheckBox *m_showPasswordCheck = nullptr;
};

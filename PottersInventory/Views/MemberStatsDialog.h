#pragma once

#include "FramelessDialog.h"

#include <QVector>

#include "Models/Assignment.h"
#include "Models/User.h"

class QNetworkAccessManager;
class RoleTypeController;

// "Responsibilities" modal opened by double-clicking a member's profile
// row in DateNavigationTab's results list -- a quick summary of
// everything they've ever been assigned (role breakdown + full history),
// not just what's scheduled for the currently selected Sunday.
class MemberStatsDialog : public FramelessDialog
{
    Q_OBJECT

public:
    MemberStatsDialog(
        const User &user,
        const QVector<Assignment> &assignments,
        RoleTypeController *roleTypeController,
        QNetworkAccessManager *networkManager,
        QWidget *parent = nullptr);
};

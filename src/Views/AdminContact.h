#pragma once

#include <QString>

// Who to contact about admin passwords: the Login dialog's "Forgot
// password?" link emails this address, and ChangePasswordDialog sends the
// new password here as well as to the admin's own email. Hardcoded for now.
inline const QString kAdminContactEmail = QStringLiteral("manucheremeh1995@gmail.com");

#pragma once

#include <QString>

// Who to contact about admin passwords: the Login dialog's "Forgot
// password?" link emails this address, and ChangePasswordDialog sends the
// new password here (members have phone numbers, not email addresses).
// Hardcoded for now.
inline const QString kAdminContactEmail = QStringLiteral("support@pottershouse.fr");

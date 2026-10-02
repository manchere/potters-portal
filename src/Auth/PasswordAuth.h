#pragma once

#include <QString>

// Password hashing and bearer-token generation for the new auth feature
// (SCHEDULING_FUNCTIONAL_REQUIREMENTS.md FR-0.1/FR-0.2). No bcrypt/argon2
// or OpenSSL dev headers are available anywhere in this repo's vcpkg
// install, but Qt Core itself ships QPasswordDigestor, so passwords are
// hashed with PBKDF2-HMAC-SHA256 through that instead of a hand-rolled KDF
// or a new dependency.
namespace PasswordAuth
{
    QString generateSalt();
    QString hashPassword(const QString &password, const QString &salt);
    bool verifyPassword(const QString &password, const QString &salt, const QString &hash);

    // Opaque, high-entropy bearer token for mobile sessions. Only its
    // SHA-256 (hashToken) is ever stored server-side -- the raw token is
    // returned to the client once, at login, and never persisted.
    QString generateToken();
    QString hashToken(const QString &token);
}

#pragma once

#include <QHttpServerRequest>

#include "Controllers/SessionController.h"
#include "Controllers/UserController.h"
#include "Models/User.h"

// Raw bearer token from the "Authorization: Bearer <token>" header, or an
// empty string if missing/malformed.
QString bearerToken(const QHttpServerRequest &request);

// QHttpServer has no middleware/interceptor chain, so every protected
// route calls this inline at the top of its lambda, the same way existing
// routes inline their own validation (see ServerMain.cpp). Resolves the
// bearer token to a session and then a user. Returns false (outUser left
// default-constructed) if the header is missing/malformed or the token is
// unknown/expired.
bool requireAuth(const QHttpServerRequest &request, SessionController &sessions, UserController &users, User &outUser);

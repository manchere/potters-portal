#include "AuthMiddleware.h"

namespace
{
    constexpr auto kBearerPrefix = "Bearer ";
}

QString bearerToken(const QHttpServerRequest &request)
{
    const QByteArray header = request.value(QByteArrayLiteral("Authorization"));
    if (!header.startsWith(kBearerPrefix)) {
        return QString();
    }
    return QString::fromUtf8(header.mid(qstrlen(kBearerPrefix)));
}

bool requireAuth(const QHttpServerRequest &request, SessionController &sessions, UserController &users, User &outUser)
{
    const QString token = bearerToken(request);
    if (token.isEmpty()) {
        return false;
    }
    const int userId = sessions.userIdForToken(token);
    if (userId < 0) {
        return false;
    }
    outUser = users.userById(userId);
    return outUser.id() >= 0;
}

#pragma once

#include <QString>

// A user account doubles as a Member profile (SCHEDULING_FUNCTIONAL_REQUIREMENTS.md
// FR-1.1/FR-1.3): name + an optional Admin flag is all a profile needs
// beyond login credentials. color is the Member's chosen profile color
// (one of Models/MemberColors), shown behind their initials. teamId is
// the Team they belong to, or -1 for none.
class User
{
public:
    User() = default;
    User(int id, QString name, QString email, bool isAdmin, QString color);

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }

    QString email() const { return m_email; }
    void setEmail(const QString &email) { m_email = email; }

    // Only ever read/written by UserController/PasswordAuth. Never
    // serialized to JSON (see Json::userToJson).
    QString passwordHash() const { return m_passwordHash; }
    void setPasswordHash(const QString &passwordHash) { m_passwordHash = passwordHash; }

    QString passwordSalt() const { return m_passwordSalt; }
    void setPasswordSalt(const QString &passwordSalt) { m_passwordSalt = passwordSalt; }

    bool isAdmin() const { return m_isAdmin; }
    void setIsAdmin(bool isAdmin) { m_isAdmin = isAdmin; }

    QString color() const { return m_color; }
    void setColor(const QString &color) { m_color = color; }

    int teamId() const { return m_teamId; }
    void setTeamId(int teamId) { m_teamId = teamId; }

private:
    int m_id = -1;
    QString m_name;
    QString m_email;
    QString m_passwordHash;
    QString m_passwordSalt;
    bool m_isAdmin = false;
    QString m_color;
    int m_teamId = -1;
};

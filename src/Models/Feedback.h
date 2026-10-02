#pragma once

#include <QDateTime>
#include <QString>

// A request sent from the desktop Feedback tab: a bug report, a feature
// request, or a change wanted on a member profile. memberId is who sent it
// (-1 if they didn't say); Admins mark it done once handled.
class Feedback
{
public:
    enum class Kind
    {
        Bug,
        Feature,
        Profile,
    };

    Feedback() = default;

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    Kind kind() const { return m_kind; }
    void setKind(Kind kind) { m_kind = kind; }

    int memberId() const { return m_memberId; }
    void setMemberId(int memberId) { m_memberId = memberId; }

    QString subject() const { return m_subject; }
    void setSubject(const QString &subject) { m_subject = subject; }

    QString details() const { return m_details; }
    void setDetails(const QString &details) { m_details = details; }

    bool isDone() const { return m_done; }
    void setDone(bool done) { m_done = done; }

    QDateTime createdAt() const { return m_createdAt; }
    void setCreatedAt(const QDateTime &createdAt) { m_createdAt = createdAt; }

    // How kind is stored in the feedback table ("bug", "feature", "profile").
    static QString kindKey(Kind kind);
    static Kind kindFromKey(const QString &key);

private:
    int m_id = -1;
    Kind m_kind = Kind::Bug;
    int m_memberId = -1;
    QString m_subject;
    QString m_details;
    bool m_done = false;
    QDateTime m_createdAt;
};

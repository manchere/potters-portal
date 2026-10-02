#include "SmtpMail.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QSslSocket>
#include <QTimer>

namespace
{
    constexpr int kTimeoutMs = 20000;

    QString setting(const char *name, const QString &fallback = QString())
    {
        const QString value = qEnvironmentVariable(name).trimmed();
        return value.isEmpty() ? fallback : value;
    }

    // One SMTP conversation. Waits use a local event loop rather than
    // QAbstractSocket::waitFor*(), which Qt documents as unreliable on
    // Windows.
    class Session
    {
    public:
        QString error;

        bool connectTo(const QString &host, quint16 port)
        {
            m_socket.connectToHostEncrypted(host, port);
            if (!waitUntil([this]() { return m_socket.isEncrypted(); })) {
                return fail(QStringLiteral("Couldn't open a secure connection to %1:%2 (%3).")
                    .arg(host).arg(port).arg(m_socket.errorString()));
            }
            return expect(220);
        }

        // Sends one command line and checks the reply code.
        bool command(const QByteArray &line, int expectedCode)
        {
            m_socket.write(line + "\r\n");
            return expect(expectedCode);
        }

        void quit()
        {
            m_socket.write("QUIT\r\n");
            m_socket.flush();
            m_socket.disconnectFromHost();
        }

    private:
        QSslSocket m_socket;

        bool fail(const QString &message)
        {
            if (error.isEmpty()) {
                error = message;
            }
            return false;
        }

        template <typename Done>
        bool waitUntil(Done done)
        {
            QElapsedTimer elapsed;
            elapsed.start();
            while (!done()) {
                // Once a connection is under way, falling back to
                // Unconnected means it failed or the server hung up.
                const qint64 remaining = kTimeoutMs - elapsed.elapsed();
                if (remaining <= 0 || m_socket.state() == QAbstractSocket::UnconnectedState) {
                    return false;
                }
                QEventLoop loop;
                QObject::connect(&m_socket, &QSslSocket::readyRead, &loop, &QEventLoop::quit);
                QObject::connect(&m_socket, &QSslSocket::encrypted, &loop, &QEventLoop::quit);
                QObject::connect(&m_socket, &QSslSocket::errorOccurred, &loop, &QEventLoop::quit);
                QObject::connect(&m_socket, &QSslSocket::disconnected, &loop, &QEventLoop::quit);
                QTimer::singleShot(remaining, &loop, &QEventLoop::quit);
                loop.exec();
            }
            return true;
        }

        // Reads a (possibly multi-line, "250-...") reply and checks its code.
        bool expect(int expectedCode)
        {
            QByteArray line;
            do {
                if (!waitUntil([this]() { return m_socket.canReadLine(); })) {
                    return fail(QStringLiteral("The mail server stopped responding (%1).").arg(m_socket.errorString()));
                }
                line = m_socket.readLine().trimmed();
            } while (line.size() > 3 && line.at(3) == '-');

            if (line.left(3).toInt() != expectedCode) {
                return fail(QStringLiteral("The mail server refused the message: %1").arg(QString::fromUtf8(line)));
            }
            return true;
        }
    };

    QByteArray buildMessage(const QString &from, const QStringList &recipients, const QString &subject, const QString &body)
    {
        QStringList lines = {
            QStringLiteral("From: Potters Portal <%1>").arg(from),
            QStringLiteral("To: %1").arg(recipients.join(QStringLiteral(", "))),
            QStringLiteral("Subject: %1").arg(subject),
            QStringLiteral("Date: %1").arg(QDateTime::currentDateTime().toString(Qt::RFC2822Date)),
            QStringLiteral("MIME-Version: 1.0"),
            QStringLiteral("Content-Type: text/plain; charset=utf-8"),
            QStringLiteral("Content-Transfer-Encoding: 8bit"),
            QString(),
        };
        for (QString line : body.split(QLatin1Char('\n'))) {
            line.remove(QLatin1Char('\r'));
            // Dot-stuffing: a lone "." would end the message early.
            if (line.startsWith(QLatin1Char('.'))) {
                line.prepend(QLatin1Char('.'));
            }
            lines << line;
        }
        return lines.join(QStringLiteral("\r\n")).toUtf8() + "\r\n.";
    }
}

namespace SmtpMail
{
    bool isConfigured()
    {
        return !setting("SMTP_USERNAME").isEmpty() && !setting("SMTP_PASSWORD").isEmpty();
    }

    bool send(const QStringList &recipients, const QString &subject, const QString &body, QString *errorMessage)
    {
        auto failWith = [errorMessage](const QString &message) {
            if (errorMessage) {
                *errorMessage = message;
            }
            return false;
        };
        if (!isConfigured()) {
            return failWith(QStringLiteral("Email isn't set up: set SMTP_USERNAME and SMTP_PASSWORD (a Gmail app password)."));
        }
        if (!QSslSocket::supportsSsl()) {
            return failWith(QStringLiteral("This build of the app can't make secure connections, so it can't send email."));
        }
        if (recipients.isEmpty()) {
            return failWith(QStringLiteral("No one to send the email to."));
        }

        const QString username = setting("SMTP_USERNAME");
        const QString password = setting("SMTP_PASSWORD");
        const QString host = setting("SMTP_HOST", QStringLiteral("smtp.gmail.com"));
        const quint16 port = static_cast<quint16>(setting("SMTP_PORT", QStringLiteral("465")).toUInt());

        Session session;
        bool ok = session.connectTo(host, port)
            && session.command("EHLO potters-portal", 250)
            && session.command("AUTH LOGIN", 334)
            && session.command(username.toUtf8().toBase64(), 334)
            && session.command(password.toUtf8().toBase64(), 235)
            && session.command("MAIL FROM:<" + username.toUtf8() + ">", 250);
        for (const QString &recipient : recipients) {
            ok = ok && session.command("RCPT TO:<" + recipient.toUtf8() + ">", 250);
        }
        ok = ok
            && session.command("DATA", 354)
            && session.command(buildMessage(username, recipients, subject, body), 250);
        session.quit();

        if (!ok) {
            QString reason = session.error;
            if (reason.contains(QLatin1String("535"))) {
                reason = QStringLiteral("Gmail rejected the login. Check SMTP_USERNAME and that SMTP_PASSWORD is an app password.");
            }
            return failWith(reason);
        }
        return true;
    }
}

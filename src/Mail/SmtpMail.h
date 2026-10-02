#pragma once

#include <QString>
#include <QStringList>

// Sends plain-text email over SMTP with implicit TLS (port 465), set up
// for Gmail. Settings come from environment variables so no credentials
// live in the code:
//   SMTP_USERNAME  the sending account, e.g. someone@gmail.com
//   SMTP_PASSWORD  a Gmail *app password* (Google Account > Security >
//                  App passwords), not the account's normal password
//   SMTP_HOST      optional, defaults to smtp.gmail.com
//   SMTP_PORT      optional, defaults to 465
// send() blocks (running a local event loop, like GroqVisionClient) until
// the server accepts the message or something fails.
namespace SmtpMail
{
    // True when SMTP_USERNAME and SMTP_PASSWORD are both set.
    bool isConfigured();

    // On failure returns false with *errorMessage set to a user-facing
    // reason.
    bool send(const QStringList &recipients, const QString &subject, const QString &body, QString *errorMessage);
}

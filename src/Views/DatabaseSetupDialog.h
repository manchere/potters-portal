#pragma once

#include "FramelessDialog.h"

class QCheckBox;
class QLabel;
class QLineEdit;

// Shown at startup when there's no working database connection -- on a
// fresh install (no DATABASE_URL and nothing saved yet), or when the saved
// connection stops working. Asks for the Postgres connection string, tries
// it, and only closes once it connects (Connect) or the person gives up
// (Quit). Portal.cpp saves a string that worked for next time. Also takes
// an optional Groq API key (photo auto-fill and search questions), saved
// alongside it; see Vision/GroqApiKey.h.
class DatabaseSetupDialog : public FramelessDialog
{
    Q_OBJECT

public:
    // problem: why it's being shown (e.g. the last connection error), or
    // empty on a first launch. initialUrl and initialGroqKey prefill the
    // fields.
    DatabaseSetupDialog(const QString &problem, const QString &initialUrl, const QString &initialGroqKey,
                        QWidget *parent = nullptr);

    // The connection string that connected; valid after exec() returns
    // QDialog::Accepted.
    QString connectionUrl() const;
    // What's in the Groq key field (may be empty -- it's optional).
    QString groqApiKey() const;

private slots:
    void connectClicked();

private:
    QLineEdit *m_urlEdit = nullptr;
    QCheckBox *m_showCheck = nullptr;
    QLineEdit *m_groqKeyEdit = nullptr;
    QLabel *m_errorLabel = nullptr;
};

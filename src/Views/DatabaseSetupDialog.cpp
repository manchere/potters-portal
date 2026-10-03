#include "DatabaseSetupDialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "Database/Database.h"

DatabaseSetupDialog::DatabaseSetupDialog(const QString &problem, const QString &initialUrl,
                                         const QString &initialGroqKey, QWidget *parent)
    : FramelessDialog(parent)
{
    setWindowTitle(tr("Connect to the Database"));
    auto *heading = new QLabel(tr("Connect to the Database"), this);
    heading->setObjectName(QStringLiteral("pageTitle"));
    auto *intro = new QLabel(
        tr("Potters Portal keeps its data in an online database. Paste the connection string your "
           "Admin gave you -- it looks like postgresql://user:password@host/neondb?sslmode=require. "
           "It's saved on this computer for your Windows account, so you only need to do this once."),
        this);
    intro->setObjectName(QStringLiteral("pageSubtitle"));
    intro->setWordWrap(true);

    m_urlEdit = new QLineEdit(initialUrl, this);
    m_urlEdit->setEchoMode(QLineEdit::Password); // it contains the password
    m_urlEdit->setPlaceholderText(QStringLiteral("postgresql://user:password@host/neondb?sslmode=require"));
    m_urlEdit->setMinimumWidth(460);
    m_showCheck = new QCheckBox(tr("Show"), this);
    connect(m_showCheck, &QCheckBox::toggled, this, [this](bool show) {
        m_urlEdit->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
        m_groqKeyEdit->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
    });

    auto *groqLabel = new QLabel(tr("Groq API key (optional)"), this);
    auto *groqHint = new QLabel(
        tr("Fills in an item's details from its photo, and answers questions in the inventory search. "
           "Leave it empty to do without; get a key at console.groq.com."),
        this);
    groqHint->setObjectName(QStringLiteral("pageSubtitle"));
    groqHint->setWordWrap(true);
    m_groqKeyEdit = new QLineEdit(initialGroqKey, this);
    m_groqKeyEdit->setEchoMode(QLineEdit::Password);
    m_groqKeyEdit->setPlaceholderText(QStringLiteral("gsk_..."));

    m_errorLabel = new QLabel(problem, this);
    m_errorLabel->setObjectName(QStringLiteral("fieldError"));
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    connect(m_urlEdit, &QLineEdit::textChanged, m_errorLabel, &QLabel::clear);

    auto *buttons = new QDialogButtonBox(this);
    QPushButton *connectButton = buttons->addButton(tr("Connect"), QDialogButtonBox::AcceptRole);
    connectButton->setDefault(true);
    QPushButton *quitButton = buttons->addButton(tr("Quit"), QDialogButtonBox::RejectRole);
    quitButton->setObjectName(QStringLiteral("secondaryButton"));
    connect(connectButton, &QPushButton::clicked, this, &DatabaseSetupDialog::connectClicked);
    connect(quitButton, &QPushButton::clicked, this, &QDialog::reject);

    auto *layout = contentLayout();
    layout->setSpacing(10);
    layout->addWidget(heading);
    layout->addWidget(intro);
    layout->addWidget(m_urlEdit);
    layout->addWidget(groqLabel);
    layout->addWidget(groqHint);
    layout->addWidget(m_groqKeyEdit);
    layout->addWidget(m_showCheck);
    layout->addWidget(m_errorLabel);
    layout->addWidget(buttons);
}

QString DatabaseSetupDialog::connectionUrl() const
{
    return m_urlEdit->text().trimmed();
}

QString DatabaseSetupDialog::groqApiKey() const
{
    return m_groqKeyEdit->text().trimmed();
}

void DatabaseSetupDialog::connectClicked()
{
    const QString url = connectionUrl();
    if (!url.startsWith(QLatin1String("postgres://")) && !url.startsWith(QLatin1String("postgresql://"))) {
        m_errorLabel->setText(tr("That doesn't look like a connection string -- it should start with postgresql://"));
        return;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString error;
    const bool ok = Database::connect(url, &error);
    QApplication::restoreOverrideCursor();
    if (!ok) {
        m_errorLabel->setText(tr("Couldn't connect: %1").arg(error));
        return;
    }
    accept();
}

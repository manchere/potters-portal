#include "SongEditDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QUrl>
#include <QVBoxLayout>

SongEditDialog::SongEditDialog(const Song &song, QWidget *parent)
    : FramelessDialog(parent)
    , m_id(song.id())
{
    const QString title = song.id() < 0 ? QStringLiteral("Add Song") : QStringLiteral("Edit Song");
    setWindowTitle(title);
    auto *heading = new QLabel(title, this);
    heading->setObjectName(QStringLiteral("pageTitle"));

    auto errorLabel = [this]() {
        auto *label = new QLabel(this);
        label->setObjectName(QStringLiteral("fieldError"));
        label->setWordWrap(true);
        return label;
    };

    m_titleEdit = new QLineEdit(song.title(), this);
    m_titleEdit->setPlaceholderText(QStringLiteral("e.g. Way Maker"));
    connect(m_titleEdit, &QLineEdit::textChanged, this, [this]() { m_titleError->clear(); });
    m_titleError = errorLabel();

    m_artistEdit = new QLineEdit(song.artist(), this);
    m_artistEdit->setPlaceholderText(QStringLiteral("e.g. Sinach"));

    m_keyEdit = new QLineEdit(song.songKey(), this);
    m_keyEdit->setPlaceholderText(QStringLiteral("e.g. G"));
    m_keyEdit->setMaximumWidth(120);

    m_linkEdit = new QLineEdit(song.link(), this);
    m_linkEdit->setPlaceholderText(QStringLiteral("https://www.youtube.com/watch?v=..."));
    connect(m_linkEdit, &QLineEdit::textChanged, this, [this]() { m_linkError->clear(); });
    m_linkError = errorLabel();

    m_lyricsEdit = new QPlainTextEdit(song.lyrics(), this);
    m_lyricsEdit->setPlaceholderText(QStringLiteral("Lyrics or notes (optional)"));
    m_lyricsEdit->setMinimumSize(420, 180);

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("Title"), m_titleEdit);
    form->addRow(QString(), m_titleError);
    form->addRow(QStringLiteral("Artist"), m_artistEdit);
    form->addRow(QStringLiteral("Key"), m_keyEdit);
    form->addRow(QStringLiteral("Play link"), m_linkEdit);
    form->addRow(QString(), m_linkError);
    form->addRow(QStringLiteral("Lyrics"), m_lyricsEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &SongEditDialog::saveClicked);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = contentLayout();
    layout->addWidget(heading);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

void SongEditDialog::saveClicked()
{
    bool valid = true;
    if (m_titleEdit->text().trimmed().isEmpty()) {
        m_titleError->setText(QStringLiteral("Title is required."));
        valid = false;
    }
    const QString link = m_linkEdit->text().trimmed();
    if (!link.isEmpty()) {
        const QUrl url(link, QUrl::StrictMode);
        const QString scheme = url.scheme().toLower();
        if (!url.isValid() || url.host().isEmpty() || (scheme != QLatin1String("http") && scheme != QLatin1String("https"))) {
            m_linkError->setText(QStringLiteral("Enter a full web link starting with https://"));
            valid = false;
        }
    }
    if (valid) {
        accept();
    }
}

Song SongEditDialog::song() const
{
    return Song(
        m_id,
        m_titleEdit->text().trimmed(),
        m_artistEdit->text().trimmed(),
        m_keyEdit->text().trimmed(),
        m_linkEdit->text().trimmed(),
        m_lyricsEdit->toPlainText().trimmed());
}

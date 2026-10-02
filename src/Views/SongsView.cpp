#include "SongsView.h"

#include <QDesktopServices>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#include "Controllers/SongController.h"
#include "SongEditDialog.h"

namespace
{
    // e.g. "Sinach  ·  Key of G" -- whichever parts are filled in.
    QString songMeta(const Song &song)
    {
        QStringList parts;
        if (!song.artist().isEmpty()) {
            parts << song.artist();
        }
        if (!song.songKey().isEmpty()) {
            parts << QStringLiteral("Key of %1").arg(song.songKey());
        }
        return parts.join(QStringLiteral("  ·  "));
    }
}

SongsView::SongsView(SongController *songController, QWidget *parent)
    : QWidget(parent)
    , m_songController(songController)
{
    auto *title = new QLabel(QStringLiteral("Songs"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(
        QStringLiteral("The church's song library. Pick a song to see its key and lyrics, "
                        "or press Play to open it."),
        this);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("Search by title or artist..."));
    connect(m_searchEdit, &QLineEdit::textChanged, this, &SongsView::rebuildList);

    m_addButton = new QPushButton(QStringLiteral("+  Add Song"), this);
    connect(m_addButton, &QPushButton::clicked, this, &SongsView::addClicked);

    auto *topRow = new QHBoxLayout;
    topRow->addWidget(m_searchEdit, 1);
    topRow->addWidget(m_addButton);

    m_list = new QListWidget(this);
    m_list->setAlternatingRowColors(true);
    connect(m_list, &QListWidget::currentRowChanged, this, &SongsView::selectionChanged);
    connect(m_list, &QListWidget::itemDoubleClicked, this, [this]() {
        if (m_isAdmin) {
            editClicked();
        } else {
            playClicked();
        }
    });
    auto *listBox = new QGroupBox(QStringLiteral("Library"), this);
    auto *listLayout = new QVBoxLayout(listBox);
    listLayout->addWidget(m_list);
    listBox->setMinimumWidth(280);

    // Detail panel: song title, artist/key, Play, lyrics, Edit/Delete.
    m_detailPanel = new QWidget(this);
    m_titleLabel = new QLabel(m_detailPanel);
    m_titleLabel->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: 700;"));
    m_titleLabel->setWordWrap(true);
    m_metaLabel = new QLabel(m_detailPanel);
    m_metaLabel->setObjectName(QStringLiteral("mutedLabel"));
    m_playButton = new QPushButton(QStringLiteral("▶  Play"), m_detailPanel);
    connect(m_playButton, &QPushButton::clicked, this, &SongsView::playClicked);
    m_lyricsView = new QPlainTextEdit(m_detailPanel);
    m_lyricsView->setReadOnly(true);
    m_lyricsView->setPlaceholderText(QStringLiteral("No lyrics added for this song."));

    m_editButton = new QPushButton(QStringLiteral("Edit"), m_detailPanel);
    m_editButton->setObjectName(QStringLiteral("secondaryButton"));
    connect(m_editButton, &QPushButton::clicked, this, &SongsView::editClicked);
    m_deleteButton = new QPushButton(QStringLiteral("Delete"), m_detailPanel);
    m_deleteButton->setObjectName(QStringLiteral("dangerButton"));
    connect(m_deleteButton, &QPushButton::clicked, this, &SongsView::deleteClicked);

    auto *playRow = new QHBoxLayout;
    playRow->addWidget(m_playButton);
    playRow->addStretch();
    auto *adminRow = new QHBoxLayout;
    adminRow->addWidget(m_editButton);
    adminRow->addWidget(m_deleteButton);
    adminRow->addStretch();

    auto *detailLayout = new QVBoxLayout(m_detailPanel);
    detailLayout->setContentsMargins(0, 0, 0, 0);
    detailLayout->addWidget(m_titleLabel);
    detailLayout->addWidget(m_metaLabel);
    detailLayout->addLayout(playRow);
    detailLayout->addWidget(m_lyricsView, 1);
    detailLayout->addLayout(adminRow);

    m_emptyLabel = new QLabel(QStringLiteral("Select a song to see its details."), this);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setObjectName(QStringLiteral("mutedLabel"));

    auto *detailBox = new QGroupBox(QStringLiteral("Song"), this);
    auto *detailBoxLayout = new QVBoxLayout(detailBox);
    detailBoxLayout->addWidget(m_emptyLabel, 1);
    detailBoxLayout->addWidget(m_detailPanel, 1);

    auto *columns = new QHBoxLayout;
    columns->setSpacing(16);
    columns->addWidget(listBox, 2);
    columns->addWidget(detailBox, 3);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(6);
    layout->addLayout(topRow);
    layout->addLayout(columns, 1);

    setAdminMode(false);
    refresh();
}

void SongsView::setAdminMode(bool isAdmin)
{
    m_isAdmin = isAdmin;
    m_addButton->setVisible(isAdmin);
    m_editButton->setVisible(isAdmin);
    m_deleteButton->setVisible(isAdmin);
}

void SongsView::refresh()
{
    const int previouslySelected = selectedSongId();
    m_songs = m_songController->allSongs();
    rebuildList();
    selectSong(previouslySelected);
}

void SongsView::rebuildList()
{
    const int previouslySelected = selectedSongId();
    const QString search = m_searchEdit->text().trimmed();

    m_list->blockSignals(true);
    m_list->clear();
    for (const Song &song : m_songs) {
        if (!search.isEmpty()
            && !song.title().contains(search, Qt::CaseInsensitive)
            && !song.artist().contains(search, Qt::CaseInsensitive)) {
            continue;
        }
        const QString meta = songMeta(song);
        auto *item = new QListWidgetItem(
            meta.isEmpty() ? song.title() : song.title() + QLatin1Char('\n') + meta, m_list);
        item->setData(Qt::UserRole, song.id());
    }
    if (m_list->count() == 0) {
        auto *item = new QListWidgetItem(
            m_songs.isEmpty() ? QStringLiteral("No songs yet.") : QStringLiteral("No songs match your search."),
            m_list);
        item->setFlags(Qt::NoItemFlags);
        item->setData(Qt::UserRole, -1);
    }
    m_list->blockSignals(false);

    selectSong(previouslySelected);
}

int SongsView::selectedSongId() const
{
    const QListWidgetItem *item = m_list->currentItem();
    return item ? item->data(Qt::UserRole).toInt() : -1;
}

void SongsView::selectSong(int id)
{
    m_list->blockSignals(true);
    m_list->setCurrentRow(-1);
    for (int i = 0; i < m_list->count(); ++i) {
        if (id >= 0 && m_list->item(i)->data(Qt::UserRole).toInt() == id) {
            m_list->setCurrentRow(i);
            break;
        }
    }
    m_list->blockSignals(false);
    selectionChanged();
}

void SongsView::selectionChanged()
{
    const int id = selectedSongId();
    for (const Song &song : m_songs) {
        if (song.id() == id) {
            showSong(song);
            return;
        }
    }
    m_detailPanel->hide();
    m_emptyLabel->show();
}

void SongsView::showSong(const Song &song)
{
    m_emptyLabel->hide();
    m_detailPanel->show();
    m_titleLabel->setText(song.title());
    const QString meta = songMeta(song);
    m_metaLabel->setText(meta);
    m_metaLabel->setVisible(!meta.isEmpty());
    m_playButton->setEnabled(!song.link().isEmpty());
    m_playButton->setToolTip(song.link().isEmpty() ? QStringLiteral("No play link added") : song.link());
    m_lyricsView->setPlainText(song.lyrics());
}

void SongsView::playClicked()
{
    const int id = selectedSongId();
    for (const Song &song : m_songs) {
        if (song.id() != id || song.link().isEmpty()) {
            continue;
        }
        if (!QDesktopServices::openUrl(QUrl(song.link()))) {
            QMessageBox::warning(this, QStringLiteral("Play"), QStringLiteral("Couldn't open %1").arg(song.link()));
        }
        return;
    }
}

void SongsView::addClicked()
{
    if (!m_isAdmin) {
        return;
    }
    SongEditDialog dialog(Song(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    Song newSong = dialog.song();
    if (!m_songController->addSong(newSong)) {
        QMessageBox::critical(this, QStringLiteral("Add Song"), m_songController->lastError());
        return;
    }
    // Clear any filter so the new song is visible, then select it.
    m_searchEdit->blockSignals(true);
    m_searchEdit->clear();
    m_searchEdit->blockSignals(false);
    refresh();
    selectSong(newSong.id());
}

void SongsView::editClicked()
{
    if (!m_isAdmin) {
        return;
    }
    const Song existing = m_songController->songById(selectedSongId());
    if (existing.id() < 0) {
        return;
    }
    SongEditDialog dialog(existing, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    if (!m_songController->updateSong(dialog.song())) {
        QMessageBox::critical(this, QStringLiteral("Edit Song"), m_songController->lastError());
        return;
    }
    refresh();
}

void SongsView::deleteClicked()
{
    if (!m_isAdmin) {
        return;
    }
    const Song existing = m_songController->songById(selectedSongId());
    if (existing.id() < 0) {
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("Delete Song"),
            QStringLiteral("Delete \"%1\" from the song library?").arg(existing.title()))
        != QMessageBox::Yes) {
        return;
    }
    if (!m_songController->removeSong(existing.id())) {
        QMessageBox::critical(this, QStringLiteral("Delete Song"), m_songController->lastError());
        return;
    }
    refresh();
}

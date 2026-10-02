#include "SongController.h"

#include <QSqlError>
#include <QSqlQuery>

#include "Database/Database.h"

namespace
{
    const QString kSelectColumns = QStringLiteral("id, title, artist, song_key, link, lyrics");

    // A null QString binds as SQL NULL, but the optional columns are
    // NOT NULL DEFAULT '' -- so an empty artist/key/link/lyrics must be
    // sent as an empty (non-null) string.
    QString orEmpty(const QString &value)
    {
        return value.isNull() ? QStringLiteral("") : value;
    }
}

SongController::SongController(QObject *parent)
    : QObject(parent)
{
}

static Song songFromQuery(const QSqlQuery &query)
{
    return Song(
        query.value(QStringLiteral("id")).toInt(),
        query.value(QStringLiteral("title")).toString(),
        query.value(QStringLiteral("artist")).toString(),
        query.value(QStringLiteral("song_key")).toString(),
        query.value(QStringLiteral("link")).toString(),
        query.value(QStringLiteral("lyrics")).toString());
}

QVector<Song> SongController::allSongs() const
{
    QVector<Song> songs;
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT %1 FROM songs ORDER BY LOWER(title)").arg(kSelectColumns));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return songs;
    }
    while (query.next()) {
        songs.append(songFromQuery(query));
    }
    return songs;
}

Song SongController::songById(int id) const
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT %1 FROM songs WHERE id = :id").arg(kSelectColumns));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return Song();
    }
    return songFromQuery(query);
}

bool SongController::addSong(Song &song)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT INTO songs (title, artist, song_key, link, lyrics) "
        "VALUES (:title, :artist, :song_key, :link, :lyrics) RETURNING id"));
    query.bindValue(QStringLiteral(":title"), song.title());
    query.bindValue(QStringLiteral(":artist"), orEmpty(song.artist()));
    query.bindValue(QStringLiteral(":song_key"), orEmpty(song.songKey()));
    query.bindValue(QStringLiteral(":link"), orEmpty(song.link()));
    query.bindValue(QStringLiteral(":lyrics"), orEmpty(song.lyrics()));
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return false;
    }
    song.setId(query.value(0).toInt());
    emit songsChanged();
    return true;
}

bool SongController::updateSong(const Song &song)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "UPDATE songs SET title = :title, artist = :artist, song_key = :song_key, link = :link, "
        "lyrics = :lyrics, updated_at = now() WHERE id = :id"));
    query.bindValue(QStringLiteral(":title"), song.title());
    query.bindValue(QStringLiteral(":artist"), orEmpty(song.artist()));
    query.bindValue(QStringLiteral(":song_key"), orEmpty(song.songKey()));
    query.bindValue(QStringLiteral(":link"), orEmpty(song.link()));
    query.bindValue(QStringLiteral(":lyrics"), orEmpty(song.lyrics()));
    query.bindValue(QStringLiteral(":id"), song.id());
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    emit songsChanged();
    return true;
}

bool SongController::removeSong(int id)
{
    Database::ensureConnected();
    QSqlQuery query;
    query.prepare(QStringLiteral("DELETE FROM songs WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    emit songsChanged();
    return true;
}

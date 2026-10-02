#pragma once

#include <QObject>
#include <QVector>

#include "Models/Song.h"

// Backed by Postgres (songs table). See ItemController for the query
// pattern.
class SongController : public QObject
{
    Q_OBJECT

public:
    explicit SongController(QObject *parent = nullptr);

    // Alphabetical by title.
    QVector<Song> allSongs() const;
    Song songById(int id) const;

    QString lastError() const { return m_lastError; }

public slots:
    bool addSong(Song &song);
    bool updateSong(const Song &song);
    bool removeSong(int id);

signals:
    void songsChanged();

private:
    mutable QString m_lastError;
};

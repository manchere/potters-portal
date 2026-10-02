#pragma once

#include <QString>

// A song in the church's worship song library (desktop Songs tab). link is
// a play link (YouTube, Spotify, ...) and songKey the musical key the team
// sings it in; both, like artist and lyrics, may be empty.
class Song
{
public:
    Song() = default;
    Song(int id, QString title, QString artist, QString songKey, QString link, QString lyrics);

    int id() const { return m_id; }
    void setId(int id) { m_id = id; }

    QString title() const { return m_title; }
    void setTitle(const QString &title) { m_title = title; }

    QString artist() const { return m_artist; }
    void setArtist(const QString &artist) { m_artist = artist; }

    QString songKey() const { return m_songKey; }
    void setSongKey(const QString &songKey) { m_songKey = songKey; }

    QString link() const { return m_link; }
    void setLink(const QString &link) { m_link = link; }

    QString lyrics() const { return m_lyrics; }
    void setLyrics(const QString &lyrics) { m_lyrics = lyrics; }

private:
    int m_id = -1;
    QString m_title;
    QString m_artist;
    QString m_songKey;
    QString m_link;
    QString m_lyrics;
};

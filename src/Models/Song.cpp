#include "Song.h"

Song::Song(int id, QString title, QString artist, QString songKey, QString link, QString lyrics)
    : m_id(id)
    , m_title(std::move(title))
    , m_artist(std::move(artist))
    , m_songKey(std::move(songKey))
    , m_link(std::move(link))
    , m_lyrics(std::move(lyrics))
{
}

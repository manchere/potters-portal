#pragma once

#include <QVector>
#include <QWidget>

#include "Models/Song.h"

class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class SongController;

// "Songs" tab -- the church's worship song library. A searchable list on
// the left (title, artist, key), and the selected song's details on the
// right with a Play button that opens its link in the browser.
//
// Viewing is open to everyone; adding, editing, and deleting songs are
// Admin-only and hidden until Admin mode is unlocked (FR-0.4), the same
// as scheduling on the Schedule tab.
class SongsView : public QWidget
{
    Q_OBJECT

public:
    explicit SongsView(SongController *songController, QWidget *parent = nullptr);

public slots:
    void refresh();
    void setAdminMode(bool isAdmin);

private slots:
    void addClicked();
    void editClicked();
    void deleteClicked();
    void playClicked();
    void selectionChanged();
    void rebuildList();

private:
    void showSong(const Song &song);
    // -1 when nothing is selected.
    int selectedSongId() const;
    void selectSong(int id);

    SongController *m_songController = nullptr;
    QVector<Song> m_songs;
    bool m_isAdmin = false;

    QLineEdit *m_searchEdit = nullptr;
    QPushButton *m_addButton = nullptr;
    QListWidget *m_list = nullptr;

    QWidget *m_detailPanel = nullptr;
    QLabel *m_emptyLabel = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_metaLabel = nullptr;
    QPushButton *m_playButton = nullptr;
    QPlainTextEdit *m_lyricsView = nullptr;
    QPushButton *m_editButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
};

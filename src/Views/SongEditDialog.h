#pragma once

#include "FramelessDialog.h"

#include "Models/Song.h"

class QLabel;
class QLineEdit;
class QPlainTextEdit;

// Add/edit a song in the worship song library (SongsView). Pass an
// existing Song to edit, or a default-constructed Song() (id() < 0) for
// "new", mirroring TagEditDialog/CategoryEditDialog. Only the title is
// required; a link, if given, must be an http(s) URL so "Play" can open
// it in the browser.
class SongEditDialog : public FramelessDialog
{
    Q_OBJECT

public:
    SongEditDialog(const Song &song, QWidget *parent = nullptr);

    Song song() const;

private slots:
    void saveClicked();

private:
    int m_id = -1;
    QLineEdit *m_titleEdit = nullptr;
    QLabel *m_titleError = nullptr;
    QLineEdit *m_artistEdit = nullptr;
    QLineEdit *m_keyEdit = nullptr;
    QLineEdit *m_linkEdit = nullptr;
    QLabel *m_linkError = nullptr;
    QPlainTextEdit *m_lyricsEdit = nullptr;
};

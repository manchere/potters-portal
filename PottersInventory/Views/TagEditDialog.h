#pragma once

#include "FramelessDialog.h"

#include "Models/Tag.h"

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

// Add/edit a tag: name + color + description. Used by AdminOverviewView's
// Taxonomy tab (replacing the old inline chip-editing form) -- pass an
// existing Tag to edit, or a default-constructed Tag() (id() < 0) for
// "new", mirroring CategoryEditDialog.
class TagEditDialog : public FramelessDialog
{
    Q_OBJECT

public:
    TagEditDialog(const Tag &tag, QWidget *parent = nullptr);

    Tag tag() const;

private slots:
    void pickColor();
    void saveClicked();

private:
    void updateColorSwatch();

    int m_id = -1;
    QString m_color;
    QLineEdit *m_nameEdit = nullptr;
    QLabel *m_nameError = nullptr;
    QPushButton *m_colorButton = nullptr;
    QPlainTextEdit *m_descriptionEdit = nullptr;
};

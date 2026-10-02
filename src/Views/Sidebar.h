#pragma once

#include <QFrame>

class QButtonGroup;
class QVBoxLayout;

// Left-hand navigation: one button per page (Schedule, Reports, ...),
// stacked top to bottom. Exactly one is checked at a time; MainWindow
// switches its page stack on currentChanged().
class Sidebar : public QFrame
{
    Q_OBJECT

public:
    explicit Sidebar(QWidget *parent = nullptr);

    // Appends a button; `icon` is a short glyph shown before the label.
    // The first page added starts checked.
    void addPage(const QString &icon, const QString &label);

    // Hides or shows a page's button (e.g. a tab someone has no access to).
    void setPageVisible(int index, bool visible);

    int currentIndex() const;
    void setCurrentIndex(int index);

signals:
    void currentChanged(int index);

private:
    QButtonGroup *m_group = nullptr;
    QVBoxLayout *m_buttonLayout = nullptr;
};

#pragma once

#include <QFrame>

class QVBoxLayout;

// Vertical column of a page's action buttons, shown at the page's right
// edge (Schedule, Reports, Songs, Items, Taxonomy). Buttons stretch to the
// bar's width and keep their own styles (primary / secondaryButton /
// dangerButton). The bar hides itself while none of its widgets are
// visible -- e.g. on the Admin-only Schedule actions when logged out.
class ActionBar : public QFrame
{
    Q_OBJECT

public:
    explicit ActionBar(QWidget *parent = nullptr);

    // Reparents `widget` into the bar, below what's already there.
    void addWidget(QWidget *widget);
    // A thin divider between groups of actions.
    void addSeparator();
    // Pushes everything added afterwards to the bottom (used for Delete).
    void addStretch();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void updateVisibility();

    QVBoxLayout *m_layout = nullptr;
    QList<QWidget *> m_widgets;
};

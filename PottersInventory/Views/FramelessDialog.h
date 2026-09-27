#pragma once

#include <QDialog>

class QVBoxLayout;

// Shared base for every modal dialog in this app: frameless (no native
// close/maximize/minimize chrome), a translucent white rounded card look,
// and centered over its parent whenever shown.
//
// The QDialog itself stays fully transparent (WA_TranslucentBackground)
// and paints nothing; only an inner child "card" widget (#dialogCard in
// Style.cpp) renders the rounded, semi-transparent background. This
// mirrors MainWindow's own frameless-window trick (see ResizeFrame): a
// translucent *top-level* widget painting its own stylesheet background
// is unreliable on Windows (confirmed by testing -- the layered-window
// surface stayed fully see-through), but an ordinary non-top-level child
// widget inside it paints its stylesheet background reliably. Subclasses
// must add their content to contentLayout(), not build a layout directly
// on `this`.
class FramelessDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FramelessDialog(QWidget *parent = nullptr);

protected:
    QVBoxLayout *contentLayout() const { return m_cardLayout; }

    void showEvent(QShowEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QWidget *m_card = nullptr;
    QVBoxLayout *m_cardLayout = nullptr;
};

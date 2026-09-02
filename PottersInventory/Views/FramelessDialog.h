#pragma once

#include <QDialog>

// Shared base for every modal dialog in this app: frameless (no native
// close/maximize/minimize chrome -- consistent with MainWindow's own
// custom TitleBar), a rounded "card" look via QSS (see Style.cpp's
// "dialogCard" rule), and centered over its parent whenever shown rather
// than wherever the OS happens to place it. Subclasses still need their
// own Save/Cancel/Close QDialogButtonBox for dismissal, and should add an
// in-content title label since the native title bar text no longer shows.
class FramelessDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FramelessDialog(QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
};

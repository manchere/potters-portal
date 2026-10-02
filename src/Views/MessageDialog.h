#pragma once

#include "FramelessDialog.h"

#include <QDialogButtonBox>
#include <QMessageBox>

class QAbstractButton;
class QLabel;
class QPushButton;

// The app's own message box: the same frameless card as every other
// dialog (FramelessDialog), with an icon, a heading, the message, optional
// smaller detail text, and the app's button styles -- instead of the
// native QMessageBox, which kept the system frame and look.
//
// For the usual cases use the static helpers, which mirror QMessageBox's
// (same arguments, same StandardButton results), e.g.
//   if (MessageDialog::question(this, tr("Delete Song"), tr("Delete it?")) != QMessageBox::Yes)
// For custom buttons, build one, addButton() each, exec(), then check
// clickedButton().
class MessageDialog : public FramelessDialog
{
    Q_OBJECT

public:
    enum class Kind
    {
        Question,
        Information,
        Warning,
        Critical,
    };

    MessageDialog(Kind kind, const QString &title, const QString &text, QWidget *parent = nullptr);

    // Smaller, muted text under the message.
    void setInformativeText(const QString &text);

    // The first button added is the default (Enter) unless
    // setDefaultButton() says otherwise. AcceptRole/YesRole buttons look
    // primary, DestructiveRole ones red, the rest secondary.
    QPushButton *addButton(const QString &text, QDialogButtonBox::ButtonRole role);
    QPushButton *addButton(QDialogButtonBox::StandardButton button);
    void setDefaultButton(QPushButton *button);

    // nullptr if the dialog was closed with Escape.
    QAbstractButton *clickedButton() const { return m_clicked; }

    static QMessageBox::StandardButton question(
        QWidget *parent, const QString &title, const QString &text,
        QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No);
    static QMessageBox::StandardButton information(QWidget *parent, const QString &title, const QString &text);
    static QMessageBox::StandardButton warning(QWidget *parent, const QString &title, const QString &text);
    static QMessageBox::StandardButton critical(QWidget *parent, const QString &title, const QString &text);

private:
    static QMessageBox::StandardButton show(Kind kind, QWidget *parent, const QString &title, const QString &text,
                                            QMessageBox::StandardButtons buttons);
    void styleButton(QPushButton *button, QDialogButtonBox::ButtonRole role);

    QLabel *m_informative = nullptr;
    QDialogButtonBox *m_buttons = nullptr;
    QAbstractButton *m_clicked = nullptr;
    bool m_hasDefault = false;
};

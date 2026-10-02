#pragma once

#include <QList>
#include <QWidget>

#include "Style.h"

class QFrame;
class QLabel;
class QPushButton;
class QRadioButton;

// "Settings" page: the app's look (Light, Black, or Navy & Gold -- applied
// as soon as one is picked) and changing the Admin password. Only reports
// intent; MainWindow applies the theme and opens the password dialog.
class SettingsView : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsView(QWidget *parent = nullptr);

    // Ticks the card for `theme` without emitting themeChosen().
    void setCurrentTheme(Theme theme);

    // The password button only works while an Admin is logged in.
    void setAdminMode(bool isAdmin);

signals:
    void themeChosen(Theme theme);
    void changePasswordClicked();

private:
    QFrame *buildThemeCard(Theme theme, const QString &description, const QStringList &swatches);
    void updateCardHighlight();

    struct ThemeOption
    {
        Theme theme;
        QFrame *card;
        QRadioButton *radio;
    };
    QList<ThemeOption> m_themeOptions;

    QPushButton *m_passwordButton = nullptr;
    QLabel *m_passwordHint = nullptr;
};

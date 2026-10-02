#pragma once

#include <QList>
#include <QWidget>

#include "Language.h"
#include "Style.h"

class QFrame;
class QGroupBox;
class QLabel;
class QPushButton;
class QRadioButton;

// "Settings" page: the app's look (Light, Black, or Navy & Gold -- applied
// as soon as one is picked), its language (English or French -- used from
// the next launch), and, only while an Admin is logged in,
// changing the Admin password. Only reports intent; MainWindow applies
// the theme and opens the password dialog.
class SettingsView : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsView(QWidget *parent = nullptr);

    // Ticks the card for `theme` without emitting themeChosen().
    void setCurrentTheme(Theme theme);

    // Ticks `language` without emitting languageChosen().
    void setCurrentLanguage(Language language);

    // The Admin password section only appears while an Admin is logged in.
    void setAdminMode(bool isAdmin);

signals:
    void themeChosen(Theme theme);
    void languageChosen(Language language);
    void changePasswordClicked();
    void accessRightsClicked();

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

    QLabel *m_subtitle = nullptr;
    QRadioButton *m_englishRadio = nullptr;
    QRadioButton *m_frenchRadio = nullptr;
    QGroupBox *m_passwordBox = nullptr;
    QGroupBox *m_accessBox = nullptr;
};

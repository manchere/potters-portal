#pragma once

#include <QString>

// Single stylesheet applied to the whole app (see Portal.cpp) so
// every view shares the same look without repeating QSS per widget. The
// same stylesheet is filled in with one of three color sets, picked on the
// Settings page: the original light theme, a black one, or navy & gold
// taken from the app icon (pottershouse.jpg).
enum class Theme
{
    Light,
    Black,
    Navy,
};

QString appStyleSheet(Theme theme);

// Name shown on the Settings page, e.g. "Navy & Gold".
QString themeDisplayName(Theme theme);

// True for the themes with light text on a dark background (Black, Navy)
// -- for the few places that color things outside the stylesheet, like
// the Sunday list rows and the report HTML.
bool isDarkTheme(Theme theme);

// The theme last picked by the user (Light until they change it), kept in
// QSettings so the app reopens the way it was left.
Theme savedTheme();

// Applies the theme app-wide (stylesheet, plus a matching palette/style so
// native widgets such as message boxes and scroll bars follow along) and
// remembers it for the next launch.
void applyTheme(Theme theme);

Theme currentTheme();

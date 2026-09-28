#pragma once

#include <QString>

// Single stylesheet applied to the whole app (see PottersInventory.cpp) so
// every view shares the same look without repeating QSS per widget. The
// same stylesheet is filled in with one of two color sets: the original
// light theme or a black one, toggled from the title bar.
enum class Theme
{
    Light,
    Black,
};

QString appStyleSheet(Theme theme);

// The theme last picked by the user (Light until they change it), kept in
// QSettings so the app reopens the way it was left.
Theme savedTheme();

// Applies the theme app-wide (stylesheet, plus a matching palette/style so
// native widgets such as message boxes and scroll bars follow along) and
// remembers it for the next launch.
void applyTheme(Theme theme);

Theme currentTheme();

#include "Style.h"

#include <QApplication>
#include <QHash>
#include <QPalette>
#include <QRegularExpression>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>

namespace
{
    // Every color the stylesheet uses, by role. The light values are the
    // app's original look; the black ones keep the navy/gold brand accents
    // but lift them where they'd vanish against black (navy text becomes
    // white, focus rings turn gold).
    QHash<QString, QString> themeColors(Theme theme)
    {
        if (theme == Theme::Navy) {
            // The app icon: a deep navy field with gold strokes. Surfaces
            // step up from the darkest navy; gold is the primary/accent
            // color, with navy text on gold buttons.
            return {
                {QStringLiteral("text"), QStringLiteral("#e8ecf4")},
                {QStringLiteral("strongText"), QStringLiteral("#ffffff")},
                {QStringLiteral("softText"), QStringLiteral("#cdd5e3")},
                {QStringLiteral("mutedText"), QStringLiteral("#9aa7bf")},
                {QStringLiteral("faintText"), QStringLiteral("#7886a0")},
                {QStringLiteral("windowBg"), QStringLiteral("#0b1a33")},
                {QStringLiteral("windowBorder"), QStringLiteral("#1f3a66")},
                {QStringLiteral("dialogCard"), QStringLiteral("rgba(16, 36, 74, 0.96)")},
                {QStringLiteral("dialogCardBorder"), QStringLiteral("rgba(232, 183, 48, 0.30)")},
                {QStringLiteral("surface"), QStringLiteral("#10244a")},
                {QStringLiteral("surfaceAlt"), QStringLiteral("#132a54")},
                {QStringLiteral("input"), QStringLiteral("#0e2142")},
                {QStringLiteral("border"), QStringLiteral("#1f3a66")},
                {QStringLiteral("inputBorder"), QStringLiteral("#2a4777")},
                {QStringLiteral("dashedBorder"), QStringLiteral("#34507f")},
                {QStringLiteral("divider"), QStringLiteral("#1a3360")},
                {QStringLiteral("hover"), QStringLiteral("#17305c")},
                {QStringLiteral("subtle"), QStringLiteral("#162d57")},
                {QStringLiteral("subtleHover"), QStringLiteral("#1f3a69")},
                {QStringLiteral("tabSelected"), QStringLiteral("#1d3a6b")},
                {QStringLiteral("tabSelectedText"), QStringLiteral("#f0c75e")},
                {QStringLiteral("selection"), QStringLiteral("#2a4a80")},
                {QStringLiteral("selectionText"), QStringLiteral("#ffffff")},
                {QStringLiteral("focus"), QStringLiteral("#e8b730")},
                {QStringLiteral("primary"), QStringLiteral("#d4a72c")},
                {QStringLiteral("primaryHover"), QStringLiteral("#e3b843")},
                {QStringLiteral("primaryPressed"), QStringLiteral("#b88f1f")},
                {QStringLiteral("primaryText"), QStringLiteral("#0b1a33")},
                {QStringLiteral("primaryDisabled"), QStringLiteral("#3a4a66")},
                {QStringLiteral("primaryDisabledText"), QStringLiteral("#8090a8")},
                {QStringLiteral("error"), QStringLiteral("#f87171")},
                {QStringLiteral("accentBg"), QStringLiteral("#2e2f2a")},
                {QStringLiteral("accentText"), QStringLiteral("#f0c75e")},
                {QStringLiteral("cardGrid"), QStringLiteral("#0b1a33")},
            };
        }
        if (theme == Theme::Black) {
            return {
                {QStringLiteral("text"), QStringLiteral("#e6e6e6")},
                {QStringLiteral("strongText"), QStringLiteral("#ffffff")},
                {QStringLiteral("softText"), QStringLiteral("#cfcfcf")},
                {QStringLiteral("mutedText"), QStringLiteral("#9a9a9a")},
                {QStringLiteral("faintText"), QStringLiteral("#7a7a7a")},
                {QStringLiteral("windowBg"), QStringLiteral("#000000")},
                {QStringLiteral("windowBorder"), QStringLiteral("#2a2a2a")},
                {QStringLiteral("dialogCard"), QStringLiteral("rgba(14, 14, 14, 0.94)")},
                {QStringLiteral("dialogCardBorder"), QStringLiteral("rgba(255, 255, 255, 0.14)")},
                {QStringLiteral("surface"), QStringLiteral("#0e0e0e")},
                {QStringLiteral("surfaceAlt"), QStringLiteral("#161616")},
                {QStringLiteral("input"), QStringLiteral("#121212")},
                {QStringLiteral("border"), QStringLiteral("#262626")},
                {QStringLiteral("inputBorder"), QStringLiteral("#333333")},
                {QStringLiteral("dashedBorder"), QStringLiteral("#3a3a3a")},
                {QStringLiteral("divider"), QStringLiteral("#1e1e1e")},
                {QStringLiteral("hover"), QStringLiteral("#1c1c1c")},
                {QStringLiteral("subtle"), QStringLiteral("#1a1a1a")},
                {QStringLiteral("subtleHover"), QStringLiteral("#262626")},
                {QStringLiteral("tabSelected"), QStringLiteral("#1c1c1c")},
                {QStringLiteral("tabSelectedText"), QStringLiteral("#ffffff")},
                {QStringLiteral("selection"), QStringLiteral("#1f3558")},
                {QStringLiteral("selectionText"), QStringLiteral("#ffffff")},
                {QStringLiteral("focus"), QStringLiteral("#d6a537")},
                {QStringLiteral("primary"), QStringLiteral("#1f4a85")},
                {QStringLiteral("primaryHover"), QStringLiteral("#285a9e")},
                {QStringLiteral("primaryPressed"), QStringLiteral("#173a6a")},
                {QStringLiteral("primaryText"), QStringLiteral("white")},
                {QStringLiteral("primaryDisabled"), QStringLiteral("#2a2f38")},
                {QStringLiteral("primaryDisabledText"), QStringLiteral("#6f747c")},
                {QStringLiteral("error"), QStringLiteral("#f87171")},
                {QStringLiteral("accentBg"), QStringLiteral("#2a2210")},
                {QStringLiteral("accentText"), QStringLiteral("#e0b85a")},
                {QStringLiteral("cardGrid"), QStringLiteral("#000000")},
            };
        }
        return {
            {QStringLiteral("text"), QStringLiteral("#262b3d")},
            {QStringLiteral("strongText"), QStringLiteral("#1f2430")},
            {QStringLiteral("softText"), QStringLiteral("#384057")},
            {QStringLiteral("mutedText"), QStringLiteral("#6b7280")},
            {QStringLiteral("faintText"), QStringLiteral("#8a90a0")},
            {QStringLiteral("windowBg"), QStringLiteral("#f7f8fb")},
            {QStringLiteral("windowBorder"), QStringLiteral("#e0e3ea")},
            {QStringLiteral("dialogCard"), QStringLiteral("rgba(255, 255, 255, 0.80)")},
            {QStringLiteral("dialogCardBorder"), QStringLiteral("rgba(0, 0, 0, 0.12)")},
            {QStringLiteral("surface"), QStringLiteral("white")},
            {QStringLiteral("surfaceAlt"), QStringLiteral("#fafbfd")},
            {QStringLiteral("input"), QStringLiteral("white")},
            {QStringLiteral("border"), QStringLiteral("#e7e9f0")},
            {QStringLiteral("inputBorder"), QStringLiteral("#dde1ea")},
            {QStringLiteral("dashedBorder"), QStringLiteral("#cdd3e0")},
            {QStringLiteral("divider"), QStringLiteral("#ebedf3")},
            {QStringLiteral("hover"), QStringLiteral("#f2f3f7")},
            {QStringLiteral("subtle"), QStringLiteral("#eef0f4")},
            {QStringLiteral("subtleHover"), QStringLiteral("#e2e5eb")},
            {QStringLiteral("tabSelected"), QStringLiteral("#e9edf5")},
            {QStringLiteral("tabSelectedText"), QStringLiteral("#14335c")},
            {QStringLiteral("selection"), QStringLiteral("#e9edf5")},
            {QStringLiteral("selectionText"), QStringLiteral("#14335c")},
            {QStringLiteral("focus"), QStringLiteral("#14335c")},
            {QStringLiteral("primary"), QStringLiteral("#14335c")},
            {QStringLiteral("primaryHover"), QStringLiteral("#0f2748")},
            {QStringLiteral("primaryPressed"), QStringLiteral("#0a1d36")},
            {QStringLiteral("primaryText"), QStringLiteral("white")},
            {QStringLiteral("primaryDisabled"), QStringLiteral("#9aa8bd")},
            {QStringLiteral("primaryDisabledText"), QStringLiteral("white")},
            {QStringLiteral("error"), QStringLiteral("#dc2626")},
            {QStringLiteral("accentBg"), QStringLiteral("#faf3e0")},
            {QStringLiteral("accentText"), QStringLiteral("#8a6a1a")},
            {QStringLiteral("cardGrid"), QStringLiteral("#eef0f4")},
        };
    }

    const char *kSettingsOrg = "PottersPortal";
    const char *kSettingsApp = "PottersPortal";
    // Where settings were saved before the app was renamed; only read, so
    // a theme chosen back then still applies.
    const char *kLegacySettingsName = "PottersInventory";
    const char *kThemeKey = "theme";

    // How each theme is stored in QSettings.
    QString themeKey(Theme theme)
    {
        switch (theme) {
        case Theme::Black: return QStringLiteral("black");
        case Theme::Navy: return QStringLiteral("navy");
        case Theme::Light: break;
        }
        return QStringLiteral("light");
    }

    Theme g_currentTheme = Theme::Light;
    // The platform style and palette the app started with (e.g.
    // windows11), restored when going back to Light after a dark theme
    // switched to Fusion.
    QString g_originalStyleName;
    QPalette g_originalPalette;

    // Native widgets the stylesheet doesn't reach (QMessageBox,
    // QColorDialog, scroll bars, calendar popups) draw from the palette;
    // Fusion is used for the dark themes because the Windows styles ignore
    // a dark palette.
    QPalette darkPalette(Theme theme)
    {
        const bool navy = theme == Theme::Navy;
        QPalette palette;
        const QColor text = navy ? QColor(0xe8, 0xec, 0xf4) : QColor(0xe6, 0xe6, 0xe6);
        const QColor disabledText = navy ? QColor(0x80, 0x90, 0xa8) : QColor(0x6f, 0x74, 0x7c);
        palette.setColor(QPalette::Window, navy ? QColor(0x10, 0x24, 0x4a) : QColor(0x0e, 0x0e, 0x0e));
        palette.setColor(QPalette::WindowText, text);
        palette.setColor(QPalette::Base, navy ? QColor(0x0e, 0x21, 0x42) : QColor(0x12, 0x12, 0x12));
        palette.setColor(QPalette::AlternateBase, navy ? QColor(0x13, 0x2a, 0x54) : QColor(0x16, 0x16, 0x16));
        palette.setColor(QPalette::ToolTipBase, navy ? QColor(0x17, 0x30, 0x5c) : QColor(0x1c, 0x1c, 0x1c));
        palette.setColor(QPalette::ToolTipText, text);
        palette.setColor(QPalette::PlaceholderText, navy ? QColor(0x78, 0x86, 0xa0) : QColor(0x7a, 0x7a, 0x7a));
        palette.setColor(QPalette::Text, text);
        palette.setColor(QPalette::Button, navy ? QColor(0x16, 0x2d, 0x57) : QColor(0x1a, 0x1a, 0x1a));
        palette.setColor(QPalette::ButtonText, text);
        palette.setColor(QPalette::BrightText, Qt::white);
        palette.setColor(QPalette::Light, navy ? QColor(0x2a, 0x47, 0x77) : QColor(0x2a, 0x2a, 0x2a));
        palette.setColor(QPalette::Midlight, navy ? QColor(0x1f, 0x3a, 0x66) : QColor(0x22, 0x22, 0x22));
        palette.setColor(QPalette::Mid, navy ? QColor(0x16, 0x2d, 0x57) : QColor(0x1a, 0x1a, 0x1a));
        palette.setColor(QPalette::Dark, navy ? QColor(0x08, 0x13, 0x26) : QColor(0x0a, 0x0a, 0x0a));
        palette.setColor(QPalette::Shadow, Qt::black);
        palette.setColor(QPalette::Highlight, navy ? QColor(0x2a, 0x4a, 0x80) : QColor(0x1f, 0x4a, 0x85));
        palette.setColor(QPalette::HighlightedText, Qt::white);
        palette.setColor(QPalette::Link, navy ? QColor(0xf0, 0xc7, 0x5e) : QColor(0xe0, 0xb8, 0x5a));
        palette.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
        palette.setColor(QPalette::Disabled, QPalette::Text, disabledText);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);
        return palette;
    }
}

QString appStyleSheet(Theme theme)
{
    QString qss = QStringLiteral(R"(
QWidget {
    font-family: "Segoe UI", sans-serif;
    font-size: 10pt;
    color: {{text}};
}

/* MainWindow paints transparent so only #windowFrame's rounded background
   is visible -- gives the frameless window soft corners instead of a
   square edge. Plain QDialog keeps an opaque fallback background (native
   dialogs like QMessageBox/QColorDialog); FramelessDialog-based modals
   override it via #framelessDialogRoot/#dialogCard below. */
QMainWindow {
    background: transparent;
}
QDialog {
    background: {{windowBg}};
}

/* --- Frameless window frame + custom title bar ------------------------- */
QWidget#windowFrame {
    background: {{windowBg}};
    border: 1px solid {{windowBorder}};
    border-radius: 10px;
}

/* --- Frameless modal dialogs (see Views/FramelessDialog) --------------- */
/* The QDialog itself stays fully transparent -- only the inner #dialogCard
   child widget paints the visible, translucent card (see
   FramelessDialog's class comment for why the split is necessary). */
QDialog#framelessDialogRoot {
    background: transparent;
}
QWidget#dialogCard {
    background: {{dialogCard}};
    border: 1px solid {{dialogCardBorder}};
    border-radius: 14px;
}
QWidget#titleBar {
    background: {{surface}};
    border-bottom: 1px solid {{divider}};
}
QToolButton#titleBarButton, QToolButton#titleBarCloseButton {
    background: transparent;
    border: none;
    border-radius: 4px;
    color: {{mutedText}};
    font-size: 11pt;
    padding: 6px 14px;
}
QToolButton#titleBarButton:hover {
    background: {{subtle}};
}
QToolButton#titleBarCloseButton:hover {
    background: #e5534b;
    color: white;
}

QLabel#titleBarAppName {
    color: {{strongText}};
    font-weight: 700;
    font-size: 11pt;
}

/* Page navigation (see Sidebar). Navy/gold to match the app icon
   (pottershouse.jpg): navy for the active state, a gold edge as the brand
   accent. */
QFrame#sidebar {
    background: {{surface}};
    border-right: 1px solid {{divider}};
}
QPushButton#sidebarButton {
    background: transparent;
    border: none;
    border-left: 3px solid transparent;
    border-radius: 8px;
    padding: 10px 12px;
    text-align: left;
    color: {{mutedText}};
    font-weight: 600;
}
QPushButton#sidebarButton:checked {
    background: {{tabSelected}};
    color: {{tabSelectedText}};
    border-left: 3px solid #d6a537;
}
QPushButton#sidebarButton:hover:!checked {
    background: {{hover}};
    color: {{softText}};
}

/* Each page's action buttons, in a column at its right edge (see
   ActionBar). */
QFrame#actionBar {
    background: {{surface}};
    border: 1px solid {{border}};
    border-radius: 10px;
}
QLabel#actionBarHeading {
    color: {{mutedText}};
    font-weight: 700;
    font-size: 9pt;
}
QFrame#actionBarSeparator {
    background: {{divider}};
    border: none;
}

/* Settings page theme choices (see SettingsView). */
QFrame#themeCard {
    background: {{surface}};
    border: 1px solid {{border}};
    border-radius: 10px;
}
QFrame#themeCard[selected="true"] {
    border: 2px solid {{focus}};
}

QGroupBox {
    border: 1px solid {{border}};
    border-radius: 10px;
    margin-top: 14px;
    padding-top: 12px;
    font-weight: 600;
    color: {{softText}};
    background: {{surface}};
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 12px;
    padding: 0 6px;
}

QLineEdit, QPlainTextEdit, QComboBox, QSpinBox, QDoubleSpinBox {
    border: 1px solid {{inputBorder}};
    border-radius: 6px;
    padding: 6px 9px;
    background: {{input}};
    selection-background-color: #14335c;
    selection-color: white;
}
QLineEdit:focus, QPlainTextEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {
    border: 1px solid {{focus}};
}
QComboBox::drop-down {
    border: none;
    width: 22px;
}
QComboBox QAbstractItemView, QListView#completerPopup {
    background: {{input}};
    border: 1px solid {{inputBorder}};
    selection-background-color: {{selection}};
    selection-color: {{selectionText}};
}
QListView#completerPopup::item {
    padding: 5px 8px;
}

/* --- Multi-member field (Reports) --------------------------------------- */
QFrame#memberPickerField {
    border: 1px solid {{inputBorder}};
    border-radius: 6px;
    background: {{input}};
}
QFrame#memberPickerField[focused="true"] {
    border: 1px solid {{focus}};
}
QLineEdit#memberPickerEdit, QLineEdit#memberPickerEdit:focus {
    border: none;
    background: transparent;
    padding: 3px 4px;
}
QFrame#memberChip {
    background: {{accentBg}};
    border-radius: 11px;
}
QLabel#memberChipLabel {
    color: {{accentText}};
    font-weight: 600;
}
QToolButton#memberChipRemove {
    color: {{accentText}};
    background: transparent;
    border: none;
    padding: 0 4px;
    font-weight: 700;
}
QToolButton#memberChipRemove:hover {
    color: {{error}};
}

QListWidget, QTableWidget {
    border: 1px solid {{border}};
    border-radius: 8px;
    background: {{surface}};
    alternate-background-color: {{surfaceAlt}};
    gridline-color: {{subtle}};
    selection-background-color: {{selection}};
    selection-color: {{selectionText}};
}
QListWidget::item, QTableWidget::item {
    padding: 6px;
}
QHeaderView::section {
    background: {{surfaceAlt}};
    color: {{mutedText}};
    padding: 9px 8px;
    border: none;
    border-bottom: 1px solid {{border}};
    border-right: 1px solid {{divider}};
    font-weight: 600;
}
QTableCornerButton::section {
    background: {{surfaceAlt}};
    border: none;
}

QToolTip {
    background: {{surface}};
    color: {{text}};
    border: 1px solid {{inputBorder}};
    padding: 4px 6px;
}

QPushButton {
    background: {{primary}};
    color: {{primaryText}};
    border: none;
    border-radius: 8px;
    padding: 8px 20px;
    font-weight: 600;
}
QPushButton:hover {
    background: {{primaryHover}};
}
QPushButton:pressed {
    background: {{primaryPressed}};
}
QPushButton:disabled {
    background: {{primaryDisabled}};
    color: {{primaryDisabledText}};
}

QPushButton#dangerButton {
    background: #e5534b;
}
QPushButton#dangerButton:hover {
    background: #c8433c;
}
QPushButton#dangerButton:pressed {
    background: #a8362f;
}

QPushButton#secondaryButton {
    background: {{subtle}};
    color: {{softText}};
}
QPushButton#secondaryButton:hover {
    background: {{subtleHover}};
}

QLabel#pageTitle {
    font-size: 15pt;
    font-weight: 700;
    color: {{strongText}};
}
QLabel#pageSubtitle {
    color: {{mutedText}};
}

/* --- Small inline labels shared across views --------------------------- */
QLabel#mutedLabel {
    color: {{mutedText}};
}
QLabel#accentLabel {
    color: {{accentText}};
}
QLabel#statPill {
    background: {{subtle}};
    border-radius: 9px;
    padding: 3px 10px;
    font-weight: 600;
}
QLabel#avatarPlaceholder {
    background: {{subtleHover}};
    border-radius: 16px;
}

/* Schedule duty rows (see ScheduleTab::buildRow). */
QLabel#dutyMemberName {
    color: {{strongText}};
    font-size: 10pt;
    font-weight: 700;
}
QLabel#dutyUnfilled {
    color: {{accentText}};
    font-size: 10pt;
    font-weight: 700;
    font-style: italic;
}
QLabel#dutyPill {
    background: {{subtle}};
    color: {{strongText}};
    border-radius: 12px;
    padding: 4px 12px;
    font-weight: 600;
}
QLabel#dutyCaption {
    color: {{faintText}};
    font-size: 8pt;
    font-weight: 600;
}
QLabel#dutyBackupName {
    color: {{softText}};
    font-weight: 600;
}
QToolButton#dutyEditButton {
    color: {{accentText}};
    background: transparent;
    border: 1px solid {{border}};
    border-radius: 6px;
    padding: 2px 8px;
    font-weight: 600;
}
QToolButton#dutyEditButton:hover {
    background: {{subtleHover}};
}
/* Absence requests (Admin only, see ScheduleTab::buildRequestCard and
   buildAbsenceNotes). */
QFrame#requestCard {
    background: {{surfaceAlt}};
    border: 1px solid {{border}};
    border-radius: 8px;
}
QLabel#absenceApproved {
    color: {{error}};
    font-size: 9pt;
    font-weight: 600;
}
QLabel#absencePending {
    color: {{accentText}};
    font-size: 9pt;
    font-style: italic;
}

/* --- Inline field validation errors (below the offending field) -------- */
QLabel#fieldError {
    color: {{error}};
    font-size: 9pt;
}

/* --- Photo tile (Add Item / Edit Item) ---------------------------------- */
QLabel#photoTile {
    border: 2px dashed {{dashedBorder}};
    border-radius: 10px;
    background: {{surfaceAlt}};
    color: {{faintText}};
    font-weight: 600;
}
QLabel#photoTile:hover {
    border-color: {{focus}};
    color: {{focus}};
}
QLabel#photoTile[hasImage="true"] {
    border: 1px solid {{border}};
    background: {{surface}};
}

/* --- Items: list/card view toggle --------------------------------------- */
QToolButton#viewToggleButton {
    background: {{subtle}};
    color: {{mutedText}};
    border: 1px solid {{border}};
    padding: 4px 10px;
    font-size: 9pt;
}
QToolButton#viewToggleButton[position="first"] {
    border-top-left-radius: 8px;
    border-bottom-left-radius: 8px;
    border-right: none;
}
QToolButton#viewToggleButton[position="last"] {
    border-top-right-radius: 8px;
    border-bottom-right-radius: 8px;
}
QToolButton#viewToggleButton:checked {
    background: {{primary}};
    color: {{primaryText}};
    border-color: {{primary}};
}

/* --- Items: search / question bar --------------------------------------- */
QLineEdit#searchEdit {
    padding-left: 12px;
}
QToolButton#questionModeButton {
    background: {{subtle}};
    color: {{mutedText}};
    border: 1px solid {{border}};
    border-radius: 6px;
    padding: 6px 12px;
    font-weight: 700;
}
QToolButton#questionModeButton:checked {
    background: {{accentBg}};
    color: {{accentText}};
    border-color: #d6a537;
}

/* --- Items: card grid ---------------------------------------------------- */
QWidget#cardGridContainer {
    background: {{cardGrid}};
}
QWidget#productCard {
    background: {{surface}};
    border: 1px solid {{border}};
    border-radius: 12px;
}
QWidget#productCard:hover {
    border-color: #d6a537;
}
QLabel#cardPhotoPlaceholder {
    background: {{surfaceAlt}};
    border: 1px dashed {{dashedBorder}};
    border-radius: 8px;
    color: {{faintText}};
}
QLabel#cardName {
    font-weight: 700;
    color: {{strongText}};
}
QLabel#cardQuantity {
    color: {{mutedText}};
    font-size: 9pt;
}

/* --- Item details modal (double-click) ----------------------------------- */
QLabel#detailLabel {
    color: {{mutedText}};
    font-weight: 600;
}
QLabel#detailValue {
    color: {{strongText}};
}
)");

    const QHash<QString, QString> colors = themeColors(theme);
    static const QRegularExpression token(QStringLiteral(R"(\{\{(\w+)\}\})"));
    QString result;
    qsizetype last = 0;
    for (auto it = token.globalMatch(qss); it.hasNext();) {
        const QRegularExpressionMatch match = it.next();
        result += QStringView(qss).mid(last, match.capturedStart() - last);
        Q_ASSERT_X(colors.contains(match.captured(1)), "appStyleSheet", "unknown color token");
        result += colors.value(match.captured(1));
        last = match.capturedEnd();
    }
    result += QStringView(qss).mid(last);
    return result;
}

Theme savedTheme()
{
    const QSettings settings(QString::fromLatin1(kSettingsOrg), QString::fromLatin1(kSettingsApp));
    QVariant theme = settings.value(QString::fromLatin1(kThemeKey));
    if (!theme.isValid()) {
        const QSettings legacy(QString::fromLatin1(kLegacySettingsName), QString::fromLatin1(kLegacySettingsName));
        theme = legacy.value(QString::fromLatin1(kThemeKey));
    }
    for (Theme candidate : {Theme::Black, Theme::Navy}) {
        if (theme.toString() == themeKey(candidate)) {
            return candidate;
        }
    }
    return Theme::Light;
}

QString themeDisplayName(Theme theme)
{
    switch (theme) {
    case Theme::Black: return QCoreApplication::translate("Style", "Black");
    case Theme::Navy: return QCoreApplication::translate("Style", "Navy & Gold");
    case Theme::Light: break;
    }
    return QCoreApplication::translate("Style", "Light");
}

bool isDarkTheme(Theme theme)
{
    return theme != Theme::Light;
}

void applyTheme(Theme theme)
{
    if (g_originalStyleName.isEmpty()) {
        g_originalStyleName = QApplication::style()->name();
        g_originalPalette = QApplication::palette();
    }

    if (isDarkTheme(theme)) {
        QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
        QApplication::setPalette(darkPalette(theme));
    } else {
        QApplication::setStyle(QStyleFactory::create(g_originalStyleName));
        QApplication::setPalette(g_originalPalette);
    }
    qApp->setStyleSheet(appStyleSheet(theme));
    g_currentTheme = theme;

    QSettings settings(QString::fromLatin1(kSettingsOrg), QString::fromLatin1(kSettingsApp));
    settings.setValue(QString::fromLatin1(kThemeKey), themeKey(theme));
}

Theme currentTheme()
{
    return g_currentTheme;
}

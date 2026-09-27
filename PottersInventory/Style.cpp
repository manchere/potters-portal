#include "Style.h"

QString appStyleSheet()
{
    return QStringLiteral(R"(
QWidget {
    font-family: "Segoe UI", sans-serif;
    font-size: 10pt;
    color: #262b3d;
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
    background: #f7f8fb;
}

/* --- Frameless window frame + custom title bar ------------------------- */
QWidget#windowFrame {
    background: #f7f8fb;
    border: 1px solid #e0e3ea;
    border-radius: 10px;
}

/* --- Frameless modal dialogs (see Views/FramelessDialog) --------------- */
/* The QDialog itself stays fully transparent -- only the inner #dialogCard
   child widget paints the visible, translucent white card (see
   FramelessDialog's class comment for why the split is necessary). */
QDialog#framelessDialogRoot {
    background: transparent;
}
QWidget#dialogCard {
    background: rgba(255, 255, 255, 0.80);
    border: 1px solid rgba(0, 0, 0, 0.12);
    border-radius: 14px;
}
QWidget#titleBar {
    background: white;
    border-bottom: 1px solid #ebedf3;
}
QToolButton#titleBarButton, QToolButton#titleBarCloseButton {
    background: transparent;
    border: none;
    border-radius: 4px;
    color: #4a4f58;
    font-size: 11pt;
    padding: 6px 14px;
}
QToolButton#titleBarButton:hover {
    background: #eef0f4;
}
QToolButton#titleBarCloseButton:hover {
    background: #e5534b;
    color: white;
}

/* Tab bar now lives inside the title bar row (see TitleBar). Navy/gold to
   match the app icon (pottershouse.jpg): navy for the active state, a thin
   gold underline as the brand accent. */
QTabBar#titleBarTabs {
    background: transparent;
    qproperty-drawBase: 0;
}
QTabBar#titleBarTabs::tab {
    background: transparent;
    padding: 8px 18px;
    margin: 6px 2px;
    border-radius: 8px;
    color: #6b7280;
    font-weight: 600;
}
QTabBar#titleBarTabs::tab:selected {
    background: #e9edf5;
    color: #14335c;
    border-bottom: 2px solid #d6a537;
}
QTabBar#titleBarTabs::tab:hover:!selected {
    background: #f2f3f7;
    color: #384057;
}

QGroupBox {
    border: 1px solid #e7e9f0;
    border-radius: 10px;
    margin-top: 14px;
    padding-top: 12px;
    font-weight: 600;
    color: #384057;
    background: white;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 12px;
    padding: 0 6px;
}

QLineEdit, QPlainTextEdit, QComboBox, QSpinBox, QDoubleSpinBox {
    border: 1px solid #dde1ea;
    border-radius: 6px;
    padding: 6px 9px;
    background: white;
    selection-background-color: #14335c;
    selection-color: white;
}
QLineEdit:focus, QPlainTextEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {
    border: 1px solid #14335c;
}
QComboBox::drop-down {
    border: none;
    width: 22px;
}

QListWidget, QTableWidget {
    border: 1px solid #e7e9f0;
    border-radius: 8px;
    background: white;
    alternate-background-color: #fafbfd;
    gridline-color: #eef0f4;
    selection-background-color: #e9edf5;
    selection-color: #14335c;
}
QListWidget::item, QTableWidget::item {
    padding: 6px;
}
QHeaderView::section {
    background: #fafbfd;
    color: #6b7280;
    padding: 9px 8px;
    border: none;
    border-bottom: 1px solid #e7e9f0;
    border-right: 1px solid #f1f2f6;
    font-weight: 600;
}

QPushButton {
    background: #14335c;
    color: white;
    border: none;
    border-radius: 8px;
    padding: 8px 20px;
    font-weight: 600;
}
QPushButton:hover {
    background: #0f2748;
}
QPushButton:pressed {
    background: #0a1d36;
}
QPushButton:disabled {
    background: #9aa8bd;
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
    background: #eef0f4;
    color: #384057;
}
QPushButton#secondaryButton:hover {
    background: #e2e5eb;
}

QLabel#pageTitle {
    font-size: 15pt;
    font-weight: 700;
    color: #1f2430;
}
QLabel#pageSubtitle {
    color: #6b7280;
}

/* --- Inline field validation errors (below the offending field) -------- */
QLabel#fieldError {
    color: #dc2626;
    font-size: 9pt;
}

/* --- Photo tile (Add Item / Edit Item) ---------------------------------- */
QLabel#photoTile {
    border: 2px dashed #cdd3e0;
    border-radius: 10px;
    background: #fafbfd;
    color: #8a90a0;
    font-weight: 600;
}
QLabel#photoTile:hover {
    border-color: #14335c;
    color: #14335c;
}
QLabel#photoTile[hasImage="true"] {
    border: 1px solid #e7e9f0;
    background: white;
}

/* --- Items: list/card view toggle --------------------------------------- */
QToolButton#viewToggleButton {
    background: #eef0f4;
    color: #6b7280;
    border: 1px solid #e7e9f0;
    padding: 6px 12px;
    font-size: 11pt;
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
    background: #14335c;
    color: white;
    border-color: #14335c;
}

/* --- Items: search / question bar --------------------------------------- */
QLineEdit#searchEdit {
    padding-left: 12px;
}
QToolButton#questionModeButton {
    background: #eef0f4;
    color: #6b7280;
    border: 1px solid #e7e9f0;
    border-radius: 6px;
    padding: 6px 12px;
    font-weight: 700;
}
QToolButton#questionModeButton:checked {
    background: #faf3e0;
    color: #8a6a1a;
    border-color: #d6a537;
}

/* --- Items: card grid ---------------------------------------------------- */
QWidget#cardGridContainer {
    background: #eef0f4;
}
QWidget#productCard {
    background: white;
    border: 1px solid #e7e9f0;
    border-radius: 12px;
}
QWidget#productCard:hover {
    border-color: #d6a537;
}
QLabel#cardPhotoPlaceholder {
    background: #fafbfd;
    border: 1px dashed #cdd3e0;
    border-radius: 8px;
    color: #8a90a0;
}
QLabel#cardName {
    font-weight: 700;
    color: #1f2430;
}
QLabel#cardQuantity {
    color: #6b7280;
    font-size: 9pt;
}

/* --- Item details modal (double-click) ----------------------------------- */
QLabel#detailLabel {
    color: #6b7280;
    font-weight: 600;
}
QLabel#detailValue {
    color: #1f2430;
}
)");
}

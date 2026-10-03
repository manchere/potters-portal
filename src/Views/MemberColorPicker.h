#pragma once

#include <QWidget>

class QButtonGroup;
class QToolButton;

// A row of round swatches, one per palette color (Models/MemberColors),
// then a "+" swatch that opens a color dialog for any other color; exactly
// one is selected. Used on the Add/Edit Member dialog.
class MemberColorPicker : public QWidget
{
    Q_OBJECT

public:
    explicit MemberColorPicker(QWidget *parent = nullptr);

    QString color() const;
    // A color outside the palette goes on the "+" swatch; an invalid one
    // falls back to the default color.
    void setColor(const QString &color);

signals:
    void colorChanged(const QString &color);

private:
    void chooseCustomColor();
    void setCustomColor(const QString &color);

    QButtonGroup *m_group = nullptr;
    QToolButton *m_customSwatch = nullptr;
    QString m_previousColor;
};

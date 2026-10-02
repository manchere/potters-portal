#pragma once

#include <QWidget>

class QButtonGroup;

// A row of round swatches, one per palette color (Models/MemberColors);
// exactly one is selected. Used on the Add/Edit Member dialog.
class MemberColorPicker : public QWidget
{
    Q_OBJECT

public:
    explicit MemberColorPicker(QWidget *parent = nullptr);

    QString color() const;
    // Falls back to the default color when `color` isn't in the palette.
    void setColor(const QString &color);

signals:
    void colorChanged(const QString &color);

private:
    QButtonGroup *m_group = nullptr;
};

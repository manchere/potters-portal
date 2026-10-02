#pragma once

#include <QLabel>

// A one-line label that shortens its text with a trailing "…" to whatever
// width it's given, and shows the whole text again once there's room. Its
// size hint is the full text, so layouts give it that much when they can;
// its minimum is a few characters, so it gives way first when they can't.
// The tooltip always holds the whole text.
class ElidedLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ElidedLabel(const QString &text = QString(), QWidget *parent = nullptr);

    void setFullText(const QString &text);
    QString fullText() const { return m_fullText; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void updateElidedText();

    QString m_fullText;
};

#include "ElidedLabel.h"

#include <QEvent>
#include <QFontMetrics>

ElidedLabel::ElidedLabel(const QString &text, QWidget *parent)
    : QLabel(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setFullText(text);
}

void ElidedLabel::setFullText(const QString &text)
{
    m_fullText = text;
    setToolTip(text);
    updateGeometry();
    updateElidedText();
}

QSize ElidedLabel::sizeHint() const
{
    const QMargins margins = contentsMargins();
    const QFontMetrics metrics = fontMetrics();
    return QSize(metrics.horizontalAdvance(m_fullText) + margins.left() + margins.right() + 2,
                 QLabel::sizeHint().height());
}

QSize ElidedLabel::minimumSizeHint() const
{
    const QMargins margins = contentsMargins();
    const QFontMetrics metrics = fontMetrics();
    const int shortest = std::min(metrics.horizontalAdvance(m_fullText),
                                  metrics.horizontalAdvance(QStringLiteral("Mmm…")));
    return QSize(shortest + margins.left() + margins.right() + 2, QLabel::minimumSizeHint().height());
}

void ElidedLabel::resizeEvent(QResizeEvent *event)
{
    QLabel::resizeEvent(event);
    updateElidedText();
}

void ElidedLabel::changeEvent(QEvent *event)
{
    QLabel::changeEvent(event);
    // The stylesheet can change the font (e.g. bold names), and with it
    // how much fits.
    if (event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange) {
        updateGeometry();
        updateElidedText();
    }
}

void ElidedLabel::updateElidedText()
{
    const QMargins margins = contentsMargins();
    const int available = width() - margins.left() - margins.right();
    QLabel::setText(fontMetrics().elidedText(m_fullText, Qt::ElideRight, std::max(0, available)));
}

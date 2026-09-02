#include "ItemCardWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QVBoxLayout>

#include "Badge.h"

namespace {

QString capitalize(const QString &s)
{
    return s.isEmpty() ? s : s.left(1).toUpper() + s.mid(1);
}

constexpr int PhotoSize = 160;
constexpr int PhotoCornerRadius = 12;

// "object-fit: cover" isn't something QLabel does for you — scale to fill
// then center-crop to an exact square so every card lines up evenly.
QPixmap coverScaled(const QPixmap &source, int size)
{
    if (source.isNull()) {
        return QPixmap();
    }
    const QPixmap scaled = source.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    const int x = (scaled.width() - size) / 2;
    const int y = (scaled.height() - size) / 2;
    return scaled.copy(x, y, size, size);
}

// QLabel's stylesheet border-radius clips its background/border but not a
// pixmap drawn via setPixmap(), so rounding the photo itself means baking
// the rounded-rect clip into the pixmap's alpha channel.
QPixmap roundedCorners(const QPixmap &source, int radius)
{
    if (source.isNull()) {
        return source;
    }
    QPixmap rounded(source.size());
    rounded.fill(Qt::transparent);
    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(QRectF(source.rect()), radius, radius);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, source);
    return rounded;
}

} // namespace

ItemCardWidget::ItemCardWidget(const Item &item, const QString &categoryName, const QVector<Tag> &tags,
                                const QPixmap &photo, QWidget *parent)
    : QWidget(parent)
    , m_itemId(item.id())
{
    Q_UNUSED(categoryName);

    // This widget itself *is* the styled card (rather than wrapping a
    // separate QFrame) so double-clicking anywhere over it reaches
    // mouseDoubleClickEvent() below — the purely decorative child labels
    // are made transparent to mouse events so they don't swallow the click.
    setObjectName(QStringLiteral("productCard"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedWidth(PhotoSize + 24);
    setCursor(Qt::PointingHandCursor);
    setToolTip(QStringLiteral("Double-click for details"));

    auto *photoLabel = new QLabel(this);
    photoLabel->setFixedSize(PhotoSize, PhotoSize);
    photoLabel->setAlignment(Qt::AlignCenter);
    photoLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    if (photo.isNull()) {
        photoLabel->setObjectName(QStringLiteral("cardPhotoPlaceholder"));
        photoLabel->setText(QStringLiteral("No Photo"));
    } else {
        photoLabel->setPixmap(roundedCorners(coverScaled(photo, PhotoSize), PhotoCornerRadius));
    }

    auto *nameLabel = new QLabel(this);
    nameLabel->setObjectName(QStringLiteral("cardName"));
    nameLabel->setWordWrap(true);
    nameLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    nameLabel->setText(item.name());

    auto *quantityLabel = new QLabel(QStringLiteral("Qty %1").arg(item.quantity()), this);
    quantityLabel->setObjectName(QStringLiteral("cardQuantity"));
    quantityLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto *metaRow = new QHBoxLayout;
    metaRow->addWidget(Badge::make(capitalize(itemStatusToString(item.status())), Badge::statusColor(item.status()), this));
    metaRow->addStretch();
    metaRow->addWidget(quantityLabel);

    auto *cardLayout = new QVBoxLayout(this);
    cardLayout->setContentsMargins(12, 12, 12, 12);
    cardLayout->setSpacing(8);
    cardLayout->addWidget(photoLabel, 0, Qt::AlignHCenter);
    cardLayout->addWidget(nameLabel);
    cardLayout->addLayout(metaRow);

    if (!tags.isEmpty()) {
        auto *tagDots = new QHBoxLayout;
        tagDots->setSpacing(4);
        for (const Tag &tag : tags) {
            auto *dot = new QLabel(this);
            dot->setFixedSize(10, 10);
            dot->setToolTip(tag.name());
            dot->setStyleSheet(QStringLiteral("background: %1; border-radius: 5px;").arg(tag.color()));
            tagDots->addWidget(dot);
        }
        tagDots->addStretch();
        cardLayout->addLayout(tagDots);
    }
}

void ItemCardWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked(m_itemId);
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

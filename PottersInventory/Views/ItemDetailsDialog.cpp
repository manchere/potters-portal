#include "ItemDetailsDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "Badge.h"
#include "FlowLayout.h"

namespace {

QString capitalize(const QString &s)
{
    return s.isEmpty() ? s : s.left(1).toUpper() + s.mid(1);
}

QLabel *valueLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text.isEmpty() ? QStringLiteral("—") : text, parent);
    label->setObjectName(QStringLiteral("detailValue"));
    label->setWordWrap(true);
    return label;
}

} // namespace

ItemDetailsDialog::ItemDetailsDialog(const Item &item, const QString &categoryName, const QVector<Tag> &tags,
                                      const QPixmap &photo, QWidget *parent)
    : FramelessDialog(parent)
{
    setWindowTitle(item.name());

    // Photo + name/status stacked vertically (rather than side by side) so
    // the name always has the dialog's full width to wrap into.
    auto *photoLabel = new QLabel(this);
    photoLabel->setFixedSize(120, 120);
    photoLabel->setAlignment(Qt::AlignCenter);
    if (photo.isNull()) {
        photoLabel->setObjectName(QStringLiteral("cardPhotoPlaceholder"));
        photoLabel->setText(QStringLiteral("No Photo"));
    } else {
        photoLabel->setPixmap(photo.scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    auto *nameLabel = new QLabel(item.name(), this);
    nameLabel->setObjectName(QStringLiteral("pageTitle"));
    nameLabel->setWordWrap(true);

    auto *statusRow = new QHBoxLayout;
    statusRow->addWidget(Badge::make(capitalize(itemStatusToString(item.status())), Badge::statusColor(item.status()), this));
    statusRow->addStretch();

    // QFormLayout (same as ItemFormWidget.cpp's details form) correctly
    // negotiates the label/value column widths, including wrapping long
    // values.
    auto *fieldsForm = new QFormLayout;
    fieldsForm->setSpacing(10);
    fieldsForm->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
    fieldsForm->addRow(QStringLiteral("Quantity"), valueLabel(QString::number(item.quantity()), this));
    fieldsForm->addRow(QStringLiteral("Location"), valueLabel(item.location(), this));
    fieldsForm->addRow(QStringLiteral("Category"), valueLabel(categoryName, this));
    if (!item.barcode().isEmpty()) {
        fieldsForm->addRow(QStringLiteral("Barcode"), valueLabel(item.barcode(), this));
    }
    fieldsForm->addRow(QStringLiteral("Description"), valueLabel(item.description(), this));

    if (!tags.isEmpty()) {
        auto *tagsCell = new QWidget(this);
        auto *tagsFlow = new FlowLayout(tagsCell, 0, 6, 6);
        for (const Tag &tag : tags) {
            tagsFlow->addWidget(Badge::make(tag.name(), QColor(tag.color()), tagsCell));
        }
        fieldsForm->addRow(QStringLiteral("Tags"), tagsCell);
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

    auto *layout = contentLayout();
    layout->addWidget(photoLabel, 0, Qt::AlignHCenter);
    layout->addSpacing(10);
    layout->addWidget(nameLabel);
    layout->addLayout(statusRow);
    layout->addSpacing(12);
    layout->addLayout(fieldsForm);
    layout->addStretch();
    layout->addWidget(buttons);

    // QLabel::sizeHint() for word-wrapped text is unreliable before the
    // widget has an established width (classic Qt chicken-and-egg), which
    // left QDialog's automatic sizeHint-based sizing consistently too
    // short here — force a floor tall enough for every field + the button
    // row regardless of that under-estimate.
    setMinimumSize(400, 560);
}

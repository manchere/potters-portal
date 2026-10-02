#include "ItemFormWidget.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QNetworkAccessManager>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStyle>
#include <QVBoxLayout>

#include "Badge.h"
#include "Controllers/CategoryController.h"
#include "Controllers/TagController.h"
#include "ClickableLabel.h"
#include "Vision/GroqVisionClient.h"
#include "Vision/LocationClient.h"

static QString capitalize(const QString &s)
{
    return s.isEmpty() ? s : s.left(1).toUpper() + s.mid(1);
}

ItemFormWidget::ItemFormWidget(TagController *tagController, CategoryController *categoryController,
                                bool autoFillLocation, QWidget *parent)
    : QWidget(parent)
    , m_tagController(tagController)
    , m_categoryController(categoryController)
    , m_networkManager(new QNetworkAccessManager(this))
{
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("e.g. Folding Table"));
    m_descriptionEdit = new QPlainTextEdit(this);
    m_descriptionEdit->setFixedHeight(64);
    m_descriptionEdit->setPlaceholderText(tr("Optional notes about this item"));
    m_quantitySpin = new QSpinBox(this);
    m_quantitySpin->setRange(0, 1000000);
    m_quantitySpin->setValue(1);
    m_locationEdit = new QLineEdit(this);
    m_locationEdit->setPlaceholderText(tr("e.g. Storage Room B, Shelf 3"));
    if (autoFillLocation) {
        const QString street = LocationLookup::currentStreetName(*m_networkManager);
        if (!street.isEmpty()) {
            m_locationEdit->setText(street);
        }
    }
    m_statusCombo = new QComboBox(this);
    for (ItemStatus status : allItemStatuses()) {
        m_statusCombo->addItem(Badge::statusName(status), static_cast<int>(status));
    }
    m_categoryCombo = new QComboBox(this);

    connect(m_nameEdit, &QLineEdit::textChanged, this, [this]() { m_nameError->clear(); });
    m_nameError = new QLabel(this);
    m_nameError->setObjectName(QStringLiteral("fieldError"));

    auto *detailsForm = new QFormLayout;
    detailsForm->setSpacing(10);
    detailsForm->setLabelAlignment(Qt::AlignRight);
    detailsForm->addRow(tr("Name"), m_nameEdit);
    detailsForm->addRow(QString(), m_nameError);
    detailsForm->addRow(tr("Description"), m_descriptionEdit);
    detailsForm->addRow(tr("Quantity"), m_quantitySpin);
    detailsForm->addRow(tr("Location"), m_locationEdit);
    detailsForm->addRow(tr("Status"), m_statusCombo);
    detailsForm->addRow(tr("Category"), m_categoryCombo);
    auto *detailsBox = new QGroupBox(tr("Item Details"), this);
    detailsBox->setLayout(detailsForm);

    // --- Photo -----------------------------------------------------------
    // The tile itself is the "add/change photo" control (click it to open
    // the file picker) — there is no separate "Choose Photo..." button.
    m_imagePreview = new ClickableLabel(this);
    m_imagePreview->setObjectName(QStringLiteral("photoTile"));
    m_imagePreview->setFixedSize(180, 180);
    m_imagePreview->setAlignment(Qt::AlignCenter);
    m_imagePreview->setToolTip(tr("Click to choose a photo"));
    connect(m_imagePreview, &ClickableLabel::clicked, this, &ItemFormWidget::choosePhotoClicked);

    m_autofillButton = new QPushButton(tr("Fill In Name && Description from Photo (AI)"), this);
    m_autofillButton->setEnabled(false);
    connect(m_autofillButton, &QPushButton::clicked, this, &ItemFormWidget::autofillFromPhotoClicked);

    m_autofillStatus = new QLabel(this);
    m_autofillStatus->setWordWrap(true);
    m_autofillStatus->hide();

    auto *photoButtonsLayout = new QVBoxLayout;
    photoButtonsLayout->addWidget(m_autofillButton);
    photoButtonsLayout->addWidget(m_autofillStatus);
    photoButtonsLayout->addStretch();

    auto *photoLayout = new QHBoxLayout;
    photoLayout->addWidget(m_imagePreview);
    photoLayout->addLayout(photoButtonsLayout, 1);
    auto *photoBox = new QGroupBox(tr("Photo"), this);
    photoBox->setLayout(photoLayout);

    m_tagList = new QListWidget(this);
    m_tagList->setFixedHeight(120);
    m_tagList->setAlternatingRowColors(true);

    m_newTagEdit = new QLineEdit(this);
    m_newTagEdit->setPlaceholderText(tr("New tag name"));
    auto *addTagButton = new QPushButton(tr("Add Tag"), this);
    connect(addTagButton, &QPushButton::clicked, this, &ItemFormWidget::createTagClicked);

    auto *newTagRow = new QHBoxLayout;
    newTagRow->addWidget(m_newTagEdit);
    newTagRow->addWidget(addTagButton);

    auto *tagsLayout = new QVBoxLayout;
    tagsLayout->setSpacing(8);
    tagsLayout->addWidget(m_tagList);
    tagsLayout->addLayout(newTagRow);
    auto *tagsBox = new QGroupBox(tr("Tags"), this);
    tagsBox->setLayout(tagsLayout);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(14);
    layout->addWidget(photoBox);
    layout->addWidget(detailsBox);
    layout->addWidget(tagsBox);

    refreshCategories();
    refreshTags();
    updatePreview();
}

void ItemFormWidget::refreshCategories()
{
    const int previousId = m_categoryCombo->currentData().toInt();
    m_categoryCombo->clear();
    m_categoryCombo->addItem(tr("None"), -1);
    for (const Category &category : m_categoryController->allCategories()) {
        m_categoryCombo->addItem(category.name(), category.id());
    }
    const int idx = m_categoryCombo->findData(previousId);
    m_categoryCombo->setCurrentIndex(idx >= 0 ? idx : 0);
}

void ItemFormWidget::refreshTags()
{
    QVector<int> checkedIds;
    for (int i = 0; i < m_tagList->count(); ++i) {
        QListWidgetItem *item = m_tagList->item(i);
        if (item->checkState() == Qt::Checked) {
            checkedIds.append(item->data(Qt::UserRole).toInt());
        }
    }

    m_tagList->clear();
    for (const Tag &tag : m_tagController->allTags()) {
        auto *item = new QListWidgetItem(tag.name(), m_tagList);
        item->setData(Qt::UserRole, tag.id());
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(checkedIds.contains(tag.id()) ? Qt::Checked : Qt::Unchecked);
    }
}

void ItemFormWidget::createTagClicked()
{
    const QString name = m_newTagEdit->text().trimmed();
    if (name.isEmpty()) {
        return;
    }
    Tag tag(-1, name);
    if (m_tagController->addTag(tag)) {
        m_newTagEdit->clear();
        refreshTags();
    }
}

void ItemFormWidget::choosePhotoClicked()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Choose Photo"), QString(),
        QStringLiteral("Images (*.png *.jpg *.jpeg *.gif *.webp)"));
    if (path.isEmpty()) {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Choose Photo"), tr("Could not open that file."));
        return;
    }

    m_imageData = file.readAll();
    m_imageMime = QMimeDatabase().mimeTypeForFile(path).name();
    if (!m_imageMime.startsWith(QLatin1String("image/"))) {
        m_imageMime = QStringLiteral("image/jpeg");
    }
    m_imageChanged = true;
    m_autofillButton->setEnabled(true);
    m_autofillStatus->hide();
    updatePreview();

    // Every photo pick (first one or a replacement) re-runs the AI fill-in,
    // overwriting whatever is currently in Name/Description.
    autofillFromPhotoClicked();
}

void ItemFormWidget::autofillFromPhotoClicked()
{
    if (m_imageData.isEmpty()) {
        return;
    }
    m_autofillButton->setEnabled(false);
    m_autofillStatus->setText(tr("Analyzing photo…"));
    m_autofillStatus->show();
    qApp->processEvents();

    const QString dataUrl = QStringLiteral("data:%1;base64,%2").arg(m_imageMime, QString::fromLatin1(m_imageData.toBase64()));
    QString errorMessage;
    const QJsonObject suggestion = GroqVision::describeItem(*m_networkManager, dataUrl, &errorMessage);

    if (suggestion.isEmpty()) {
        m_autofillStatus->hide();
        QMessageBox::warning(this, tr("Fill In From Photo"), errorMessage);
    } else {
        if (suggestion.contains(QStringLiteral("name"))) {
            m_nameEdit->setText(suggestion.value(QStringLiteral("name")).toString());
        }
        if (suggestion.contains(QStringLiteral("description"))) {
            m_descriptionEdit->setPlainText(suggestion.value(QStringLiteral("description")).toString());
        }
        m_autofillStatus->setText(tr("Filled in from photo — review before saving."));
    }
    m_autofillButton->setEnabled(true);
}

void ItemFormWidget::updatePreview()
{
    const bool hasImage = !m_imageData.isEmpty();
    if (!hasImage) {
        // setPixmap() internally clears any text, so only ever call one of
        // setText/setPixmap depending on state (never both).
        m_imagePreview->setText(tr("+ Add Photo"));
    } else {
        QPixmap pixmap;
        pixmap.loadFromData(m_imageData);
        m_imagePreview->setPixmap(pixmap.scaled(m_imagePreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    m_imagePreview->setProperty("hasImage", hasImage);
    m_imagePreview->style()->unpolish(m_imagePreview);
    m_imagePreview->style()->polish(m_imagePreview);
}

void ItemFormWidget::setExistingImage(const QByteArray &data, const QString &mime)
{
    m_imageData = data;
    m_imageMime = mime;
    m_imageChanged = false;
    m_autofillButton->setEnabled(!data.isEmpty());
    updatePreview();
}

bool ItemFormWidget::validate()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        m_nameError->setText(tr("Name is required."));
        return false;
    }
    return true;
}

Item ItemFormWidget::toItem() const
{
    Item item;
    item.setId(m_editingId);
    item.setName(m_nameEdit->text().trimmed());
    item.setDescription(m_descriptionEdit->toPlainText().trimmed());
    item.setQuantity(m_quantitySpin->value());
    item.setLocation(m_locationEdit->text().trimmed());
    item.setStatus(static_cast<ItemStatus>(m_statusCombo->currentData().toInt()));
    item.setCategoryId(m_categoryCombo->currentData().toInt());

    QVector<int> tagIds;
    for (int i = 0; i < m_tagList->count(); ++i) {
        QListWidgetItem *listItem = m_tagList->item(i);
        if (listItem->checkState() == Qt::Checked) {
            tagIds.append(listItem->data(Qt::UserRole).toInt());
        }
    }
    item.setTagIds(tagIds);
    return item;
}

void ItemFormWidget::setItem(const Item &item)
{
    m_editingId = item.id();
    m_nameEdit->setText(item.name());
    m_descriptionEdit->setPlainText(item.description());
    m_quantitySpin->setValue(item.quantity());
    m_locationEdit->setText(item.location());
    m_statusCombo->setCurrentIndex(m_statusCombo->findData(static_cast<int>(item.status())));
    m_categoryCombo->setCurrentIndex(m_categoryCombo->findData(item.categoryId()));

    for (int i = 0; i < m_tagList->count(); ++i) {
        QListWidgetItem *listItem = m_tagList->item(i);
        const bool checked = item.tagIds().contains(listItem->data(Qt::UserRole).toInt());
        listItem->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    }
}

void ItemFormWidget::clear()
{
    setItem(Item());
    m_quantitySpin->setValue(1);
    setExistingImage(QByteArray(), QString());
    m_autofillStatus->hide();
}

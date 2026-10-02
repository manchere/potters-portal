#pragma once

#include <QByteArray>
#include <QWidget>

#include "Models/Item.h"

class QLineEdit;
class QPlainTextEdit;
class QSpinBox;
class QComboBox;
class QListWidget;
class QLabel;
class QPushButton;
class QNetworkAccessManager;
class CategoryController;
class TagController;
class ClickableLabel;

// Shared name/description/quantity/location/status/category/tags/image form
// used by both the "Add Item" tab and the edit-item dialog opened from the
// list. Photo capture + Groq auto-fill live here so both flows get them for
// free.
class ItemFormWidget : public QWidget
{
    Q_OBJECT

public:
    // autoFillLocation: look up the current street via LocationLookup and
    // pre-fill Location with it. Only meaningful for a blank "Add Item"
    // form -- ItemEditDialog passes false and overwrites Location with the
    // existing item's value via setItem() right after construction anyway.
    ItemFormWidget(TagController *tagController, CategoryController *categoryController,
                   bool autoFillLocation = false, QWidget *parent = nullptr);

    Item toItem() const;
    void setItem(const Item &item);
    void clear();

    // Validates Name (the only required field), showing/clearing its
    // inline error label. Returns true when the form is valid to save.
    bool validate();

    // Preloads an existing item's photo (e.g. when opening the edit dialog)
    // without marking the image as "changed" — see imageChanged().
    void setExistingImage(const QByteArray &data, const QString &mime);

    // True once the user has picked a new photo via "Choose Photo..." in
    // this session (not set by setExistingImage), i.e. there's something new
    // to push through ItemController::setItemImage after saving.
    bool imageChanged() const { return m_imageChanged; }
    QByteArray imageData() const { return m_imageData; }
    QString imageMime() const { return m_imageMime; }

public slots:
    void refreshCategories();
    void refreshTags();

private slots:
    void createTagClicked();
    void choosePhotoClicked();
    void autofillFromPhotoClicked();

private:
    void updatePreview();

    TagController *m_tagController = nullptr;
    CategoryController *m_categoryController = nullptr;
    QNetworkAccessManager *m_networkManager = nullptr;

    int m_editingId = -1;
    QLineEdit *m_nameEdit = nullptr;
    QLabel *m_nameError = nullptr;
    QPlainTextEdit *m_descriptionEdit = nullptr;
    QSpinBox *m_quantitySpin = nullptr;
    QLineEdit *m_locationEdit = nullptr;
    QComboBox *m_statusCombo = nullptr;
    QComboBox *m_categoryCombo = nullptr;
    QListWidget *m_tagList = nullptr;
    QLineEdit *m_newTagEdit = nullptr;

    ClickableLabel *m_imagePreview = nullptr;
    QPushButton *m_autofillButton = nullptr;
    QLabel *m_autofillStatus = nullptr;
    QByteArray m_imageData;
    QString m_imageMime;
    bool m_imageChanged = false;
};

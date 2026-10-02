#pragma once

#include <QHash>
#include <QPixmap>
#include <QVector>
#include <QWidget>

#include "Models/AccessRights.h"
#include "Models/Category.h"
#include "Models/Item.h"
#include "Models/Tag.h"

class QTableWidget;
class QLineEdit;
class QPushButton;
class QToolButton;
class QLabel;
class QStackedWidget;
class QWidget;
class ItemController;
class TagController;
class CategoryController;
class QNetworkAccessManager;

// "Inventory" tab: a table (or card grid) of every item with actions to edit,
// delete, change status, or attach a tag. Also owns the search/question bar
// that filters whichever view is currently shown.
class ItemListView : public QWidget
{
    Q_OBJECT

public:
    ItemListView(ItemController *itemController, TagController *tagController,
                 CategoryController *categoryController, QWidget *parent = nullptr);

public slots:
    void refresh();
    // Shows Add / Edit, Add Tag, Set Status / Delete per the Inventory
    // rights in Settings > Access Rights.
    void setAccess(const SectionAccess &access);

private slots:
    void addItemClicked();
    void editClicked();
    void deleteClicked();
    void addTagClicked();
    void setStatusClicked();
    void searchTextChanged(const QString &text);
    void searchSubmitted();
    void questionModeToggled(bool enabled);
    void updateNetworkStatus();

private:
    // The structured result of either the AI or local question parser —
    // status/category are exact matches, keywords is fuzzy-matched.
    struct ItemFilter
    {
        QString status;
        QString category;
        QStringList tags;
        QString keywords;
    };

    int selectedItemId() const;
    void openItemDetails(int id);
    void rebuildTable();
    void rebuildCardGrid(const QVector<Item> &items);
    void applyFilter();
    bool passesFilter(const Item &item, const QString &plainQuery) const;
    bool passesStructuredFilter(const Item &item, const ItemFilter &filter) const;
    ItemFilter interpretQuestionLocally(const QString &question) const;

    ItemController *m_itemController = nullptr;
    TagController *m_tagController = nullptr;
    CategoryController *m_categoryController = nullptr;
    QNetworkAccessManager *m_networkManager = nullptr;

    // Cached from the last refresh() so filtering/card-building never need
    // to re-query the controllers on every keystroke.
    QVector<Item> m_items;
    QHash<int, Item> m_itemsById;
    QHash<int, QString> m_categoryNames;
    QHash<int, Tag> m_tagsById;
    QHash<int, QPixmap> m_photoCache;

    QTableWidget *m_table = nullptr;
    QWidget *m_cardContainer = nullptr;
    QStackedWidget *m_viewStack = nullptr;

    QLineEdit *m_searchEdit = nullptr;
    QToolButton *m_questionModeButton = nullptr;
    SectionAccess m_access;
    QPushButton *m_addItemButton = nullptr;
    QPushButton *m_editButton = nullptr;
    QPushButton *m_addTagButton = nullptr;
    QPushButton *m_setStatusButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QLabel *m_networkStatusLabel = nullptr;

    bool m_questionMode = false;
    bool m_structuredFilterActive = false;
    ItemFilter m_activeFilter;
};

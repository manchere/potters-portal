#include "ItemListView.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkInformation>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSet>
#include <QStackedWidget>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include "Badge.h"
#include "Controllers/CategoryController.h"
#include "Controllers/ItemController.h"
#include "Controllers/TagController.h"
#include "FlowLayout.h"
#include "ItemAddDialog.h"
#include "ItemCardWidget.h"
#include "ItemDetailsDialog.h"
#include "ItemEditDialog.h"
#include "Vision/GroqQueryClient.h"

namespace {

constexpr int ColumnSelect = 0;
constexpr int ColumnPhoto = 1;
constexpr int ColumnName = 2;
constexpr int ColumnQuantity = 3;
constexpr int ColumnLocation = 4;
constexpr int ColumnStatus = 5;
constexpr int ColumnCategory = 6;
constexpr int ColumnTags = 7;
constexpr int ColumnCount = 8;

QString capitalize(const QString &s)
{
    return s.isEmpty() ? s : s.left(1).toUpper() + s.mid(1);
}

int levenshtein(const QString &a, const QString &b)
{
    const int n = a.size();
    const int m = b.size();
    QVector<QVector<int>> dp(n + 1, QVector<int>(m + 1, 0));
    for (int i = 0; i <= n; ++i) {
        dp[i][0] = i;
    }
    for (int j = 0; j <= m; ++j) {
        dp[0][j] = j;
    }
    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            const int cost = (a[i - 1].toLower() == b[j - 1].toLower()) ? 0 : 1;
            dp[i][j] = std::min({dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost});
        }
    }
    return dp[n][m];
}

// Case-insensitive, typo-tolerant "contains": exact substring first, else
// falls back to a per-word Levenshtein distance under a length-scaled
// threshold so a slightly misspelled or partial word still matches.
bool fuzzyContains(const QString &haystack, const QString &needle)
{
    const QString n = needle.trimmed().toLower();
    if (n.isEmpty()) {
        return true;
    }
    const QString h = haystack.toLower();
    if (h.contains(n)) {
        return true;
    }
    const int threshold = n.size() <= 3 ? 1 : (n.size() <= 6 ? 2 : 3);
    const QStringList words = h.split(QRegularExpression(QStringLiteral("\\W+")), Qt::SkipEmptyParts);
    for (const QString &word : words) {
        if (qAbs(word.size() - n.size()) > threshold) {
            continue;
        }
        if (levenshtein(n, word) <= threshold) {
            return true;
        }
    }
    return false;
}

} // namespace

ItemListView::ItemListView(ItemController *itemController, TagController *tagController,
                            CategoryController *categoryController, QWidget *parent)
    : QWidget(parent)
    , m_itemController(itemController)
    , m_tagController(tagController)
    , m_categoryController(categoryController)
    , m_networkManager(new QNetworkAccessManager(this))
{
    auto *listViewButton = new QToolButton(this);
    listViewButton->setObjectName(QStringLiteral("viewToggleButton"));
    listViewButton->setProperty("position", QStringLiteral("first"));
    listViewButton->setText(QStringLiteral("List"));
    listViewButton->setCheckable(true);
    listViewButton->setToolTip(QStringLiteral("List view"));

    auto *gridViewButton = new QToolButton(this);
    gridViewButton->setObjectName(QStringLiteral("viewToggleButton"));
    gridViewButton->setProperty("position", QStringLiteral("last"));
    gridViewButton->setText(QStringLiteral("Cards"));
    gridViewButton->setCheckable(true);
    gridViewButton->setToolTip(QStringLiteral("Card view"));

    auto *viewToggleRow = new QHBoxLayout;
    viewToggleRow->setSpacing(0);
    viewToggleRow->addWidget(listViewButton);
    viewToggleRow->addWidget(gridViewButton);

    auto *title = new QLabel(QStringLiteral("Items"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(QStringLiteral("Select a row to edit, delete, add a tag, or change status."), this);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));

    auto *addItemButton = new QPushButton(QStringLiteral("+ Add Item"), this);
    connect(addItemButton, &QPushButton::clicked, this, &ItemListView::addItemClicked);

    auto *titleColumn = new QVBoxLayout;
    titleColumn->setSpacing(2);
    titleColumn->addWidget(title);
    titleColumn->addWidget(subtitle);

    auto *headerRow = new QHBoxLayout;
    headerRow->addLayout(viewToggleRow);
    headerRow->addSpacing(12);
    headerRow->addLayout(titleColumn);
    headerRow->addStretch();
    headerRow->addWidget(addItemButton);

    // --- Search / question row ------------------------------------------
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setObjectName(QStringLiteral("searchEdit"));
    m_searchEdit->setPlaceholderText(QStringLiteral("Search items..."));
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ItemListView::searchTextChanged);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &ItemListView::searchSubmitted);

    m_questionModeButton = new QToolButton(this);
    m_questionModeButton->setObjectName(QStringLiteral("questionModeButton"));
    m_questionModeButton->setText(QStringLiteral("?"));
    m_questionModeButton->setCheckable(true);
    m_questionModeButton->setToolTip(QStringLiteral("Ask a question instead of typing keywords"));
    connect(m_questionModeButton, &QToolButton::toggled, this, &ItemListView::questionModeToggled);

    m_networkStatusLabel = new QLabel(this);
    m_networkStatusLabel->setFixedSize(10, 10);

    auto *searchRow = new QHBoxLayout;
    searchRow->addWidget(m_searchEdit, 1);
    searchRow->addWidget(m_questionModeButton);
    searchRow->addSpacing(4);
    searchRow->addWidget(m_networkStatusLabel);

    QNetworkInformation::loadDefaultBackend();
    if (QNetworkInformation *net = QNetworkInformation::instance()) {
        connect(net, &QNetworkInformation::reachabilityChanged, this, &ItemListView::updateNetworkStatus);
    }
    updateNetworkStatus();

    // --- List view (table) -------------------------------------------------
    m_table = new QTableWidget(this);
    m_table->setColumnCount(ColumnCount);
    m_table->setHorizontalHeaderLabels(
        {QString(), QStringLiteral("Photo"), QStringLiteral("Name"), QStringLiteral("Quantity"), QStringLiteral("Location"),
         QStringLiteral("Status"), QStringLiteral("Category"), QStringLiteral("Tags")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(ColumnSelect, QHeaderView::Fixed);
    m_table->horizontalHeader()->resizeSection(ColumnSelect, 32);
    m_table->horizontalHeader()->setSectionResizeMode(ColumnPhoto, QHeaderView::Fixed);
    m_table->horizontalHeader()->resizeSection(ColumnPhoto, 56);
    m_table->horizontalHeader()->setSectionResizeMode(ColumnName, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(ColumnQuantity, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColumnLocation, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColumnStatus, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(48);
    m_table->setIconSize(QSize(40, 40));
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSortingEnabled(true);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        QTableWidgetItem *nameItem = m_table->item(row, ColumnName);
        if (nameItem) {
            openItemDetails(nameItem->data(Qt::UserRole).toInt());
        }
    });

    // --- Card view (grid) ----------------------------------------------
    m_cardContainer = new QWidget;
    m_cardContainer->setObjectName(QStringLiteral("cardGridContainer"));
    m_cardContainer->setAttribute(Qt::WA_StyledBackground, true);
    new FlowLayout(m_cardContainer, 16, 16, 16);
    auto *cardScroll = new QScrollArea(this);
    cardScroll->setWidgetResizable(true);
    cardScroll->setFrameShape(QFrame::NoFrame);
    cardScroll->setWidget(m_cardContainer);

    m_viewStack = new QStackedWidget(this);
    m_viewStack->addWidget(m_table);
    m_viewStack->addWidget(cardScroll);

    // Two independently checkable buttons rather than an exclusive
    // QButtonGroup, so the "uncheck the other one" behavior is explicit
    // and doesn't depend on QButtonGroup's own bookkeeping.
    connect(listViewButton, &QToolButton::toggled, this, [this, gridViewButton](bool checked) {
        if (checked) {
            gridViewButton->setChecked(false);
            m_viewStack->setCurrentIndex(0);
        }
    });
    connect(gridViewButton, &QToolButton::toggled, this, [this, listViewButton](bool checked) {
        if (checked) {
            listViewButton->setChecked(false);
            m_viewStack->setCurrentIndex(1);
        }
    });
    listViewButton->setChecked(true);

    auto *editButton = new QPushButton(QStringLiteral("Edit"), this);
    auto *addTagButton = new QPushButton(QStringLiteral("Add Tag"), this);
    auto *setStatusButton = new QPushButton(QStringLiteral("Set Status"), this);
    editButton->setObjectName(QStringLiteral("secondaryButton"));
    addTagButton->setObjectName(QStringLiteral("secondaryButton"));
    setStatusButton->setObjectName(QStringLiteral("secondaryButton"));
    auto *deleteButton = new QPushButton(QStringLiteral("Delete"), this);
    deleteButton->setObjectName(QStringLiteral("dangerButton"));
    connect(editButton, &QPushButton::clicked, this, &ItemListView::editClicked);
    connect(deleteButton, &QPushButton::clicked, this, &ItemListView::deleteClicked);
    connect(addTagButton, &QPushButton::clicked, this, &ItemListView::addTagClicked);
    connect(setStatusButton, &QPushButton::clicked, this, &ItemListView::setStatusClicked);

    auto *buttonRow = new QHBoxLayout;
    buttonRow->addWidget(editButton);
    buttonRow->addWidget(addTagButton);
    buttonRow->addWidget(setStatusButton);
    buttonRow->addStretch();
    buttonRow->addWidget(deleteButton);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    layout->addLayout(headerRow);
    layout->addLayout(searchRow);
    layout->addSpacing(6);
    layout->addWidget(m_viewStack);
    layout->addLayout(buttonRow);

    refresh();
}

void ItemListView::refresh()
{
    m_items = m_itemController->allItems();

    m_categoryNames.clear();
    for (const Category &category : m_categoryController->allCategories()) {
        m_categoryNames.insert(category.id(), category.name());
    }
    m_tagsById.clear();
    for (const Tag &tag : m_tagController->allTags()) {
        m_tagsById.insert(tag.id(), tag);
    }

    m_itemsById.clear();
    for (const Item &item : std::as_const(m_items)) {
        m_itemsById.insert(item.id(), item);
        if (!item.imageMime().isEmpty() && !m_photoCache.contains(item.id())) {
            QByteArray data;
            QString mime;
            if (m_itemController->itemImage(item.id(), data, mime)) {
                QPixmap pixmap;
                pixmap.loadFromData(data);
                m_photoCache.insert(item.id(), pixmap);
            }
        }
    }

    rebuildTable();
    applyFilter();
}

void ItemListView::rebuildTable()
{
    m_table->setSortingEnabled(false);
    m_table->setRowCount(m_items.size());
    for (int row = 0; row < m_items.size(); ++row) {
        const Item &item = m_items[row];

        // Always-visible row-selection checkbox (separate from single-row
        // click selection used by Edit/Delete/etc. below).
        auto *selectItem = new QTableWidgetItem;
        selectItem->setFlags((selectItem->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
        selectItem->setCheckState(Qt::Unchecked);
        selectItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, ColumnSelect, selectItem);

        auto *photoItem = new QTableWidgetItem;
        const QPixmap photo = m_photoCache.value(item.id());
        if (!photo.isNull()) {
            photoItem->setIcon(QIcon(photo));
        }
        m_table->setItem(row, ColumnPhoto, photoItem);

        auto *nameItem = new QTableWidgetItem(item.name());
        nameItem->setData(Qt::UserRole, item.id());
        m_table->setItem(row, ColumnName, nameItem);
        m_table->setItem(row, ColumnQuantity, new QTableWidgetItem(QString::number(item.quantity())));
        m_table->setItem(row, ColumnLocation, new QTableWidgetItem(item.location()));

        auto *statusCell = new QWidget(m_table);
        auto *statusLayout = new QHBoxLayout(statusCell);
        statusLayout->setContentsMargins(6, 4, 6, 4);
        statusLayout->addWidget(Badge::make(capitalize(itemStatusToString(item.status())), Badge::statusColor(item.status()), statusCell));
        statusLayout->addStretch();
        m_table->setCellWidget(row, ColumnStatus, statusCell);

        m_table->setItem(row, ColumnCategory, new QTableWidgetItem(m_categoryNames.value(item.categoryId(), QStringLiteral("None"))));

        auto *tagsCell = new QWidget(m_table);
        auto *tagsFlow = new FlowLayout(tagsCell, 4, 6, 6);
        for (int tagId : item.tagIds()) {
            const Tag tag = m_tagsById.value(tagId, Tag(tagId, QStringLiteral("#%1").arg(tagId)));
            tagsFlow->addWidget(Badge::make(tag.name(), QColor(tag.color()), tagsCell));
        }
        m_table->setCellWidget(row, ColumnTags, tagsCell);

        m_table->resizeRowToContents(row);
    }
    m_table->setSortingEnabled(true);
}

void ItemListView::rebuildCardGrid(const QVector<Item> &items)
{
    const auto oldCards = m_cardContainer->findChildren<ItemCardWidget *>(QString(), Qt::FindDirectChildrenOnly);
    qDeleteAll(oldCards);

    auto *flow = static_cast<FlowLayout *>(m_cardContainer->layout());
    for (const Item &item : items) {
        QVector<Tag> itemTags;
        for (int tagId : item.tagIds()) {
            if (m_tagsById.contains(tagId)) {
                itemTags << m_tagsById.value(tagId);
            }
        }
        auto *card = new ItemCardWidget(item, m_categoryNames.value(item.categoryId(), QStringLiteral("None")),
                                          itemTags, m_photoCache.value(item.id()), m_cardContainer);
        connect(card, &ItemCardWidget::doubleClicked, this, &ItemListView::openItemDetails);
        flow->addWidget(card);
    }
}

bool ItemListView::passesFilter(const Item &item, const QString &plainQuery) const
{
    if (plainQuery.trimmed().isEmpty()) {
        return true;
    }
    QStringList haystackParts{item.name(), item.description(), item.location(),
                               m_categoryNames.value(item.categoryId())};
    for (int tagId : item.tagIds()) {
        haystackParts << m_tagsById.value(tagId).name();
    }
    return fuzzyContains(haystackParts.join(QLatin1Char(' ')), plainQuery);
}

bool ItemListView::passesStructuredFilter(const Item &item, const ItemFilter &filter) const
{
    if (!filter.status.isEmpty()) {
        const QString itemStatus = capitalize(itemStatusToString(item.status()));
        if (QString::compare(itemStatus, filter.status, Qt::CaseInsensitive) != 0) {
            return false;
        }
    }
    if (!filter.category.isEmpty()) {
        const QString itemCategory = m_categoryNames.value(item.categoryId());
        if (QString::compare(itemCategory, filter.category, Qt::CaseInsensitive) != 0) {
            return false;
        }
    }
    if (!filter.tags.isEmpty()) {
        bool hasAny = false;
        for (int tagId : item.tagIds()) {
            const QString tagName = m_tagsById.value(tagId).name();
            for (const QString &wanted : filter.tags) {
                if (QString::compare(tagName, wanted, Qt::CaseInsensitive) == 0) {
                    hasAny = true;
                    break;
                }
            }
            if (hasAny) {
                break;
            }
        }
        if (!hasAny) {
            return false;
        }
    }
    if (!filter.keywords.trimmed().isEmpty()) {
        return passesFilter(item, filter.keywords);
    }
    return true;
}

void ItemListView::applyFilter()
{
    const QString query = m_searchEdit->text();

    QVector<Item> visibleItems;
    QSet<int> visibleIds;
    for (const Item &item : std::as_const(m_items)) {
        const bool visible = m_structuredFilterActive ? passesStructuredFilter(item, m_activeFilter) : passesFilter(item, query);
        if (visible) {
            visibleItems << item;
            visibleIds.insert(item.id());
        }
    }

    for (int row = 0; row < m_table->rowCount(); ++row) {
        QTableWidgetItem *nameItem = m_table->item(row, ColumnName);
        const int id = nameItem ? nameItem->data(Qt::UserRole).toInt() : -1;
        m_table->setRowHidden(row, !visibleIds.contains(id));
    }

    rebuildCardGrid(visibleItems);
}

ItemListView::ItemFilter ItemListView::interpretQuestionLocally(const QString &question) const
{
    static const QHash<QString, QString> statusAliases{
        {QStringLiteral("in stock"), QStringLiteral("Available")},
        {QStringLiteral("available"), QStringLiteral("Available")},
        {QStringLiteral("missing"), QStringLiteral("Missing")},
        {QStringLiteral("broken"), QStringLiteral("Broken")},
        {QStringLiteral("damaged"), QStringLiteral("Broken")},
        {QStringLiteral("faulty"), QStringLiteral("Broken")},
        {QStringLiteral("lost"), QStringLiteral("Lost")},
        {QStringLiteral("gone"), QStringLiteral("Lost")},
    };
    static const QSet<QString> stopWords{
        QStringLiteral("show"), QStringLiteral("me"), QStringLiteral("find"), QStringLiteral("get"),
        QStringLiteral("items"), QStringLiteral("item"), QStringLiteral("that"), QStringLiteral("are"),
        QStringLiteral("is"), QStringLiteral("the"), QStringLiteral("a"), QStringLiteral("an"),
        QStringLiteral("in"), QStringLiteral("with"), QStringLiteral("have"), QStringLiteral("has"),
        QStringLiteral("what"), QStringLiteral("which"), QStringLiteral("where"), QStringLiteral("do"),
        QStringLiteral("we"), QStringLiteral("any"), QStringLiteral("all"), QStringLiteral("for"),
        QStringLiteral("of"), QStringLiteral("this"), QStringLiteral("list"), QStringLiteral("search"),
        QStringLiteral("and"), QStringLiteral("tagged"), QStringLiteral("tag"),
    };

    ItemFilter filter;
    QString remaining = question.toLower();

    for (auto it = statusAliases.constBegin(); it != statusAliases.constEnd(); ++it) {
        if (remaining.contains(it.key())) {
            filter.status = it.value();
            remaining.replace(it.key(), QString());
            break;
        }
    }

    for (auto it = m_categoryNames.constBegin(); it != m_categoryNames.constEnd(); ++it) {
        if (!it.value().isEmpty() && fuzzyContains(remaining, it.value())) {
            filter.category = it.value();
            remaining.replace(it.value().toLower(), QString());
            break;
        }
    }

    for (auto it = m_tagsById.constBegin(); it != m_tagsById.constEnd(); ++it) {
        if (fuzzyContains(remaining, it.value().name())) {
            filter.tags << it.value().name();
            remaining.replace(it.value().name().toLower(), QString());
        }
    }

    const QStringList tokens = remaining.split(QRegularExpression(QStringLiteral("\\W+")), Qt::SkipEmptyParts);
    QStringList kept;
    for (const QString &token : tokens) {
        if (!stopWords.contains(token)) {
            kept << token;
        }
    }
    filter.keywords = kept.join(QLatin1Char(' '));
    return filter;
}

void ItemListView::searchTextChanged(const QString &text)
{
    Q_UNUSED(text);
    if (m_questionMode) {
        return;
    }
    m_structuredFilterActive = false;
    applyFilter();
}

void ItemListView::searchSubmitted()
{
    if (!m_questionMode) {
        return;
    }
    const QString question = m_searchEdit->text().trimmed();
    if (question.isEmpty()) {
        m_structuredFilterActive = false;
        applyFilter();
        return;
    }

    bool usedAi = false;
    const bool online = QNetworkInformation::instance()
        && QNetworkInformation::instance()->reachability() == QNetworkInformation::Reachability::Online;
    if (online) {
        QStringList statuses;
        for (ItemStatus status : allItemStatuses()) {
            statuses << capitalize(itemStatusToString(status));
        }
        QStringList categories = m_categoryNames.values();
        QStringList tags;
        for (const Tag &tag : std::as_const(m_tagsById)) {
            tags << tag.name();
        }

        QString error;
        setCursor(Qt::WaitCursor);
        qApp->processEvents();
        const GroqQuery::ItemFilter aiFilter =
            GroqQuery::interpretQuestion(*m_networkManager, question, statuses, categories, tags, &error);
        unsetCursor();
        if (error.isEmpty()) {
            m_activeFilter = {aiFilter.status, aiFilter.category, aiFilter.tags, aiFilter.keywords};
            usedAi = true;
        }
    }
    if (!usedAi) {
        m_activeFilter = interpretQuestionLocally(question);
    }
    m_structuredFilterActive = true;
    applyFilter();
}

void ItemListView::questionModeToggled(bool enabled)
{
    m_questionMode = enabled;
    m_searchEdit->setPlaceholderText(enabled
        ? QStringLiteral("Ask a question, e.g. \"broken items in storage\" — press Enter")
        : QStringLiteral("Search items..."));
    if (!enabled) {
        m_structuredFilterActive = false;
        applyFilter();
    }
}

void ItemListView::updateNetworkStatus()
{
    const bool online = QNetworkInformation::instance()
        && QNetworkInformation::instance()->reachability() == QNetworkInformation::Reachability::Online;
    m_networkStatusLabel->setStyleSheet(QStringLiteral("background: %1; border-radius: 5px;")
        .arg(online ? QStringLiteral("#1f8a4c") : QStringLiteral("#9aa0ab")));
    m_networkStatusLabel->setToolTip(online
        ? QStringLiteral("Online — questions are answered with AI")
        : QStringLiteral("Offline — questions use local keyword matching"));
}

int ItemListView::selectedItemId() const
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        return -1;
    }
    return m_table->item(selected.first()->row(), ColumnName)->data(Qt::UserRole).toInt();
}

void ItemListView::addItemClicked()
{
    ItemAddDialog dialog(m_itemController, m_tagController, m_categoryController, this);
    if (dialog.exec() == QDialog::Accepted) {
        refresh();
    }
}

void ItemListView::openItemDetails(int id)
{
    if (id < 0 || !m_itemsById.contains(id)) {
        return;
    }
    const Item &item = m_itemsById.value(id);
    QVector<Tag> itemTags;
    for (int tagId : item.tagIds()) {
        if (m_tagsById.contains(tagId)) {
            itemTags << m_tagsById.value(tagId);
        }
    }
    ItemDetailsDialog dialog(item, m_categoryNames.value(item.categoryId(), QStringLiteral("None")),
                              itemTags, m_photoCache.value(id), this);
    dialog.exec();
}

void ItemListView::editClicked()
{
    const int id = selectedItemId();
    if (id < 0) {
        QMessageBox::information(this, QStringLiteral("Edit Item"), QStringLiteral("Select an item first."));
        return;
    }
    ItemEditDialog dialog(m_itemController->itemById(id), m_itemController, m_tagController, m_categoryController, this);
    if (dialog.exec() == QDialog::Accepted) {
        refresh();
    }
}

void ItemListView::deleteClicked()
{
    const int id = selectedItemId();
    if (id < 0) {
        QMessageBox::information(this, QStringLiteral("Delete Item"), QStringLiteral("Select an item first."));
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("Delete Item"), QStringLiteral("Delete this item?"))
        != QMessageBox::Yes) {
        return;
    }
    if (m_itemController->removeItem(id)) {
        refresh();
    } else {
        QMessageBox::critical(this, QStringLiteral("Delete Item"), m_itemController->lastError());
    }
}

void ItemListView::addTagClicked()
{
    const int id = selectedItemId();
    if (id < 0) {
        QMessageBox::information(this, QStringLiteral("Add Tag"), QStringLiteral("Select an item first."));
        return;
    }

    const QVector<Tag> tags = m_tagController->allTags();
    QStringList names;
    for (const Tag &tag : tags) {
        names << tag.name();
    }
    if (names.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Add Tag"),
                                  QStringLiteral("No tags exist yet. Create one from the Tags & Categories tab."));
        return;
    }

    bool ok = false;
    const QString chosen = QInputDialog::getItem(this, QStringLiteral("Add Tag"), QStringLiteral("Tag"), names, 0, false, &ok);
    if (!ok) {
        return;
    }
    const int index = names.indexOf(chosen);
    if (index < 0) {
        return;
    }
    if (!m_itemController->addTagToItem(id, tags[index].id())) {
        QMessageBox::critical(this, QStringLiteral("Add Tag"), m_itemController->lastError());
        return;
    }
    refresh();
}

void ItemListView::setStatusClicked()
{
    const int id = selectedItemId();
    if (id < 0) {
        QMessageBox::information(this, QStringLiteral("Set Status"), QStringLiteral("Select an item first."));
        return;
    }

    const QVector<ItemStatus> statuses = allItemStatuses();
    QStringList names;
    for (ItemStatus status : statuses) {
        names << capitalize(itemStatusToString(status));
    }

    bool ok = false;
    const QString chosen = QInputDialog::getItem(this, QStringLiteral("Set Status"), QStringLiteral("Status"), names, 0, false, &ok);
    if (!ok) {
        return;
    }
    const int index = names.indexOf(chosen);
    if (index < 0) {
        return;
    }
    if (!m_itemController->setItemStatus(id, statuses[index])) {
        QMessageBox::critical(this, QStringLiteral("Set Status"), m_itemController->lastError());
        return;
    }
    refresh();
}

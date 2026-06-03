/*
    Copyright 2016-2025 melonDS team, DualityDS contributors

    This file is part of DualityDS, based on melonDS.

    DualityDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#include "GameLibraryWidget.h"
#include "GameLibraryModel.h"
#include "RomScanner.h"
#include "CoverFetcher.h"

#include <QCoreApplication>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListView>
#include <QPushButton>
#include <QLabel>
#include <QThread>
#include <QFileDialog>
#include <QDialog>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QDir>
#include <QUrl>

GameLibraryWidget::GameLibraryWidget(QWidget* parent)
    : QWidget(parent)
{
    RomScanner::registerTypes(); // make GameEntry usable in queued signals

    model = new GameLibraryModel(this);

    // Proxy gives alphabetical sort + live title search over the source model.
    proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(model);
    proxy->setDynamicSortFilter(true);
    proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy->setFilterRole(Qt::DisplayRole);
    proxy->sort(0);

    view = new QListView(this);
    view->setModel(proxy);
    view->setViewMode(QListView::IconMode);
    view->setIconSize(QSize(128, 128));
    view->setGridSize(QSize(160, 184));
    view->setResizeMode(QListView::Adjust);
    view->setMovement(QListView::Static);
    view->setUniformItemSizes(true);
    view->setWordWrap(true);
    view->setSpacing(8);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(view, &QListView::activated, this, &GameLibraryWidget::onItemActivated);
    connect(view, &QListView::doubleClicked, this, &GameLibraryWidget::onItemActivated);

    auto* addBtn = new QPushButton(tr("Add ROM Folder…"), this);
    auto* manageBtn = new QPushButton(tr("Manage Folders…"), this);
    auto* rescanBtn = new QPushButton(tr("Rescan"), this);
    statusLabel = new QLabel(this);

    connect(addBtn, &QPushButton::clicked, this, &GameLibraryWidget::onAddFolder);
    connect(manageBtn, &QPushButton::clicked, this, &GameLibraryWidget::onManageFolders);
    connect(rescanBtn, &QPushButton::clicked, this, &GameLibraryWidget::onRescan);

    searchBox = new QLineEdit(this);
    searchBox->setPlaceholderText(tr("Search…"));
    searchBox->setClearButtonEnabled(true);
    searchBox->setMaximumWidth(240);
    connect(searchBox, &QLineEdit::textChanged, this, [this](const QString& text) {
        proxy->setFilterFixedString(text);
    });

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(addBtn);
    toolbar->addWidget(manageBtn);
    toolbar->addWidget(rescanBtn);
    toolbar->addWidget(searchBox);
    toolbar->addStretch(1);
    toolbar->addWidget(statusLabel);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(view, 1);

    setAcceptDrops(true); // drop ROM folders or .nds files to add them

    // Worker thread for scanning so the UI never blocks on disk I/O.
    scanThread = new QThread(this);
    scanner = new RomScanner();
    scanner->moveToThread(scanThread);
    connect(scanThread, &QThread::finished, scanner, &QObject::deleteLater);
    connect(scanner, &RomScanner::entryFound, this, &GameLibraryWidget::onEntryFound);
    connect(scanner, &RomScanner::finished, this, &GameLibraryWidget::onScanFinished);
    scanThread->start();

    // Online cover art (GameTDB), cached next to the executable.
    coverFetcher = new CoverFetcher(
        QCoreApplication::applicationDirPath() + "/covers", this);
    connect(coverFetcher, &CoverFetcher::coverReady, this, [this](int index, const QString& path) {
        model->setCover(index, path);
    });

    updateEmptyState(0);
}

GameLibraryWidget::~GameLibraryWidget()
{
    scanThread->quit();
    scanThread->wait();
}

void GameLibraryWidget::setFolders(const QStringList& folders)
{
    scanFolders = folders;
    startScan();
}

void GameLibraryWidget::onAddFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select ROM Folder"));
    if (dir.isEmpty()) return;
    addFolders({dir});
}

void GameLibraryWidget::addFolders(const QStringList& newFolders)
{
    bool changed = false;
    for (const QString& f : newFolders)
    {
        if (f.isEmpty() || scanFolders.contains(f)) continue;
        scanFolders.append(f);
        changed = true;
    }
    if (!changed) return;

    emit foldersChanged(scanFolders);
    startScan();
}

void GameLibraryWidget::onManageFolders()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Manage ROM Folders"));
    dlg.resize(520, 300);

    auto* list = new QListWidget(&dlg);
    list->addItems(scanFolders);

    auto* removeBtn = new QPushButton(tr("Remove Selected"), &dlg);
    removeBtn->setEnabled(false);
    connect(list, &QListWidget::itemSelectionChanged, &dlg, [&]() {
        removeBtn->setEnabled(list->currentItem() != nullptr);
    });
    connect(removeBtn, &QPushButton::clicked, &dlg, [&]() {
        if (auto* item = list->currentItem())
            delete list->takeItem(list->row(item));
    });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    auto* row = new QHBoxLayout();
    row->addWidget(removeBtn);
    row->addStretch(1);

    auto* layout = new QVBoxLayout(&dlg);
    layout->addWidget(list, 1);
    layout->addLayout(row);
    layout->addWidget(buttons);

    if (dlg.exec() != QDialog::Accepted)
        return;

    QStringList kept;
    for (int i = 0; i < list->count(); i++)
        kept.append(list->item(i)->text());

    if (kept == scanFolders)
        return;

    scanFolders = kept;
    emit foldersChanged(scanFolders);
    startScan();
}

void GameLibraryWidget::onRescan()
{
    startScan();
}

void GameLibraryWidget::startScan()
{
    if (coverFetcher) coverFetcher->cancelAll(); // stale indices after clear
    model->clear();
    if (scanFolders.isEmpty())
    {
        updateEmptyState(0);
        return;
    }
    statusLabel->setText(tr("Scanning…"));
    // Queued call into the worker thread.
    QMetaObject::invokeMethod(scanner, "scan", Qt::QueuedConnection,
                              Q_ARG(QStringList, scanFolders));
}

void GameLibraryWidget::onEntryFound(const GameEntry& entry)
{
    model->addEntry(entry);
    // Kick off the online cover fetch for this row (cached → instant).
    coverFetcher->request(model->rowCount() - 1, entry.gameCode);
}

void GameLibraryWidget::onScanFinished(int count)
{
    updateEmptyState(count);
}

void GameLibraryWidget::updateEmptyState(int count)
{
    if (scanFolders.isEmpty())
        statusLabel->setText(tr("No ROM folder added yet — click \"Add ROM Folder…\""));
    else
        statusLabel->setText(tr("%n game(s)", "", count));
}

void GameLibraryWidget::onItemActivated(const QModelIndex& index)
{
    // `index` belongs to the proxy; data() forwards to the source model.
    QString path = index.data(GameLibraryModel::RomPathRole).toString();
    if (!path.isEmpty())
        emit gameActivated(path);
}

void GameLibraryWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void GameLibraryWidget::dropEvent(QDropEvent* event)
{
    static const QStringList romExts = {"nds", "dsi", "srl", "ids"};

    QStringList toAdd;
    for (const QUrl& url : event->mimeData()->urls())
    {
        QString local = url.toLocalFile();
        if (local.isEmpty()) continue;

        QFileInfo fi(local);
        if (fi.isDir())
            toAdd.append(fi.absoluteFilePath());
        else if (romExts.contains(fi.suffix().toLower()))
            toAdd.append(fi.absolutePath()); // add the ROM's containing folder
    }

    if (!toAdd.isEmpty())
    {
        addFolders(toAdd);
        event->acceptProposedAction();
    }
}

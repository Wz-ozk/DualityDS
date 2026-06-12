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
#include "CoverCarousel.h"
#include "RomScanner.h"
#include "CoverFetcher.h"

#include <QCoreApplication>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include <QFrame>
#include <QTimer>
#include <QTime>
#include <QThread>
#include <QFileDialog>
#include <QDialog>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QStorageInfo>
#include <QGraphicsDropShadowEffect>

namespace
{
constexpr int   BOTTOM_BAR_H = 120;
constexpr int   ARROW_SIZE   = 56;
constexpr int   ARROW_MARGIN = 16;
constexpr qreal BYTES_PER_GB = 1000.0 * 1000.0 * 1000.0;

// A large, chunky console-style round button: white/light-gray fill, blue glowing
// ring, drop shadow.
QToolButton* makeWiiButton(const QString& glyph, const QString& tip, bool enabled = true)
{
    auto* b = new QToolButton();
    b->setText(glyph);
    b->setToolTip(tip);
    b->setEnabled(enabled);
    b->setCursor(enabled ? Qt::PointingHandCursor : Qt::ArrowCursor);
    b->setFixedSize(64, 64);
    b->setStyleSheet(
        "QToolButton {"
        "  border-radius: 32px; font-size: 26px; font-weight: bold; color: #2A6FA0;"
        "  background: qradialgradient(cx:0.5, cy:0.40, radius:0.95,"
        "    stop:0 #FFFFFF, stop:1 #DCE6EF);"
        "  border: 3px solid #5BB4EE;"
        "}"
        "QToolButton:hover { border: 3px solid #7CC8F7;"
        "  background: qradialgradient(cx:0.5, cy:0.40, radius:0.95,"
        "    stop:0 #FFFFFF, stop:1 #EAF2F9); }"
        "QToolButton:pressed { background: #D2E4F2; }"
        "QToolButton:disabled { color: #9AA7B2; border: 3px solid #C2CCD4;"
        "    background: #EDEFF2; }"
        "QToolButton::menu-indicator { image: none; width: 0; height: 0; }");

    if (enabled)
    {
        auto* shadow = new QGraphicsDropShadowEffect(b);
        shadow->setBlurRadius(16);
        shadow->setOffset(0, 3);
        shadow->setColor(QColor(0, 0, 0, 80));
        b->setGraphicsEffect(shadow);
    }
    return b;
}

// A circular faceplate icon button: white fill, blue icon tint, subtle border.
QToolButton* makeIconButton(const QString& glyph, const QString& tip, bool enabled = true)
{
    auto* b = new QToolButton();
    b->setText(glyph);
    b->setToolTip(tip);
    b->setEnabled(enabled);
    b->setCursor(enabled ? Qt::PointingHandCursor : Qt::ArrowCursor);
    b->setFixedSize(44, 44);
    b->setStyleSheet(
        "QToolButton {"
        "  border-radius: 22px; font-size: 18px; color: #2E86C8;"
        "  background: #FFFFFF;"
        "  border: 1px solid #C6D2DC;"
        "}"
        "QToolButton:hover { background: #EAF4FB; border: 1px solid #5BB4EE; }"
        "QToolButton:pressed { background: #D6E9F7; }"
        "QToolButton:disabled { color: #B4C0CA; background: #F1F3F5;"
        "    border: 1px solid #D8DEE3; }");
    return b;
}
}

GameLibraryWidget::GameLibraryWidget(QWidget* parent)
    : QWidget(parent), sortAscending(true)
{
    RomScanner::registerTypes(); // make GameEntry usable in queued signals

    model = new GameLibraryModel(this);

    carousel = new CoverCarousel(model, this);
    connect(carousel, &CoverCarousel::coverActivated, this, &GameLibraryWidget::onCoverActivated);

    // Edge navigation arrows, overlaid on the carousel (positioned in resizeEvent).
    auto makeArrow = [this](const QString& glyph) {
        auto* a = new QPushButton(glyph, this);
        a->setCursor(Qt::PointingHandCursor);
        a->setFixedSize(ARROW_SIZE, ARROW_SIZE);
        a->setStyleSheet(
            "QPushButton {"
            "  border-radius: 28px; font-size: 22px; font-weight: bold; color: #2A6FA0;"
            "  background: rgba(255,255,255,0.85);"
            "  border: 1px solid rgba(0,0,0,0.10);"
            "}"
            "QPushButton:hover { background: #FFFFFF; color: #1E6FB0; }"
            "QPushButton:pressed { background: #DCEBF7; }");
        return a;
    };
    leftArrow = makeArrow(QString::fromUtf8("◀"));   // ◀
    rightArrow = makeArrow(QString::fromUtf8("▶"));  // ▶
    connect(leftArrow, &QPushButton::clicked, carousel, &CoverCarousel::prev);
    connect(rightArrow, &QPushButton::clicked, carousel, &CoverCarousel::next);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(carousel, 1);
    layout->addWidget(buildBottomBar());

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

    refreshStorageLine(0);
}

GameLibraryWidget::~GameLibraryWidget()
{
    scanThread->quit();
    scanThread->wait();
}

QFrame* GameLibraryWidget::buildBottomBar()
{
    auto* bar = new QFrame(this);
    bar->setFixedHeight(BOTTOM_BAR_H);
    bar->setStyleSheet(
        "QFrame {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #FBFCFD, stop:1 #D8DEE5);"
        "  border-top: 2px solid rgba(120,180,235,0.55);"
        "}"
        "QLabel { color: #3A4750; background: transparent; }");

    // Left Wii button → settings placeholder (empty for now).
    auto* gearBtn = makeWiiButton(QString::fromUtf8("⚙"), tr("Settings (coming soon)")); // ⚙

    clockLabel = new QLabel(bar);
    clockLabel->setStyleSheet(
        "font-size: 28px; font-weight: bold; color: #3A4750;"
        "font-family: 'Consolas', 'DejaVu Sans Mono', monospace;");
    clock = new QTimer(this);
    clock->setInterval(1000);
    connect(clock, &QTimer::timeout, this, [this]() {
        clockLabel->setText(QTime::currentTime().toString("hh:mm:ss"));
    });
    clock->start();
    clockLabel->setText(QTime::currentTime().toString("hh:mm:ss"));

    // Faceplate icon row: search decorative (disabled); sort + the folder actions
    // (add / manage / rescan) are wired up.
    auto* searchBtn = makeIconButton(QString::fromUtf8("\U0001F50D"), tr("Search (coming soon)"), false);
    auto* sortBtn   = makeIconButton(QString::fromUtf8("⇅"), tr("Sort A–Z / Z–A"), true);
    auto* addBtn    = makeIconButton(QString::fromUtf8("＋"), tr("Add ROM Folder…"), true);
    auto* manageBtn = makeIconButton(QString::fromUtf8("\U0001F4C1"), tr("Manage Folders…"), true);
    auto* rescanBtn = makeIconButton(QString::fromUtf8("⟳"), tr("Rescan"), true);
    connect(sortBtn,   &QToolButton::clicked, this, &GameLibraryWidget::onSortToggled);
    connect(addBtn,    &QToolButton::clicked, this, &GameLibraryWidget::onAddFolder);
    connect(manageBtn, &QToolButton::clicked, this, &GameLibraryWidget::onManageFolders);
    connect(rescanBtn, &QToolButton::clicked, this, &GameLibraryWidget::onRescan);

    auto* iconRow = new QHBoxLayout();
    iconRow->setSpacing(12);
    iconRow->addWidget(searchBtn);
    iconRow->addWidget(sortBtn);
    iconRow->addWidget(addBtn);
    iconRow->addWidget(manageBtn);
    iconRow->addWidget(rescanBtn);

    storageLabel = new QLabel(bar);
    storageLabel->setAlignment(Qt::AlignCenter);
    storageLabel->setStyleSheet("font-size: 12px; color: #7A8794;");

    // Icon buttons stacked over the centered storage line.
    auto* centerCol = new QVBoxLayout();
    centerCol->setSpacing(6);
    centerCol->addLayout(iconRow);
    centerCol->addWidget(storageLabel, 0, Qt::AlignHCenter);

    // Right Wii button → boot the centered game.
    auto* startBtn = makeWiiButton(QString::fromUtf8("▶"), tr("Start game")); // ▶
    connect(startBtn, &QToolButton::clicked, this, [this]() {
        if (carousel->count() > 0)
            onCoverActivated(carousel->currentIndex());
    });

    auto* row = new QHBoxLayout(bar);
    row->setContentsMargins(28, 14, 28, 14);
    row->setSpacing(18);
    row->addWidget(gearBtn);
    row->addWidget(clockLabel);
    row->addStretch(1);
    row->addLayout(centerCol);
    row->addStretch(1);
    row->addWidget(startBtn);

    return bar;
}

void GameLibraryWidget::positionArrows()
{
    // Vertically center the arrows over the carousel region (above the bottom bar).
    const int regionH = height() - BOTTOM_BAR_H;
    const int y = (regionH - ARROW_SIZE) / 2;
    leftArrow->move(ARROW_MARGIN, y);
    rightArrow->move(width() - ARROW_SIZE - ARROW_MARGIN, y);
    leftArrow->raise();
    rightArrow->raise();
}

void GameLibraryWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    positionArrows();
}

void GameLibraryWidget::refreshStorageLine(int count)
{
    if (scanFolders.isEmpty())
    {
        storageLabel->setText(tr("No ROM folder added — use ⚙ to add one"));
        return;
    }

    QStorageInfo storage(scanFolders.first());
    if (storage.isValid() && storage.bytesTotal() > 0)
    {
        const double freeGb  = storage.bytesAvailable() / BYTES_PER_GB;
        const double totalGb = storage.bytesTotal() / BYTES_PER_GB;
        storageLabel->setText(tr("%1 GB of %2 GB free  /  Games: %3")
                                  .arg(freeGb, 0, 'f', 1)
                                  .arg(totalGb, 0, 'f', 1)
                                  .arg(count));
    }
    else
    {
        storageLabel->setText(tr("Games: %1").arg(count));
    }
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

void GameLibraryWidget::onSortToggled()
{
    // In-flight cover downloads are keyed by row; sorting reorders rows, so cancel
    // them first. Already-downloaded covers live in the entries and travel along.
    if (coverFetcher) coverFetcher->cancelAll();
    sortAscending = !sortAscending;
    model->sortByTitle(sortAscending);
}

void GameLibraryWidget::startScan()
{
    if (coverFetcher) coverFetcher->cancelAll(); // stale indices after clear
    model->clear();
    if (scanFolders.isEmpty())
    {
        refreshStorageLine(0);
        return;
    }
    refreshStorageLine(0);
    // Queued call into the worker thread.
    QMetaObject::invokeMethod(scanner, "scan", Qt::QueuedConnection,
                              Q_ARG(QStringList, scanFolders));
}

void GameLibraryWidget::onEntryFound(const GameEntry& entry)
{
    model->addEntry(entry);
    // Kick off the online cover fetch for this row (cached → instant). Pass the
    // ROM basename + title so the fetcher can try high-res libretro boxart first.
    coverFetcher->request(model->rowCount() - 1, entry.gameCode,
                          QFileInfo(entry.romPath).completeBaseName(), entry.title);
}

void GameLibraryWidget::onScanFinished(int count)
{
    refreshStorageLine(count);
}

void GameLibraryWidget::onCoverActivated(int row)
{
    QString path = model->index(row).data(GameLibraryModel::RomPathRole).toString();
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

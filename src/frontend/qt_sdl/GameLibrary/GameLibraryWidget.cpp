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
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QEnterEvent>
#include <QPaintEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <QFont>

#include <SDL2/SDL.h>

namespace
{
constexpr int   BOTTOM_BAR_H = 120;
constexpr int   ARROW_SIZE   = 56;
constexpr int   ARROW_MARGIN = 16;

// Gamepad carousel navigation.
constexpr int   PAD_POLL_MS      = 30;
constexpr int   PAD_INITIAL_TICKS = 11; // ~330ms before auto-repeat kicks in
constexpr int   PAD_REPEAT_TICKS  = 5;  // ~150ms between repeats while held
constexpr int   PAD_AXIS_DEADZONE = 16000; // of 32767
constexpr qreal BYTES_PER_GB = 1000.0 * 1000.0 * 1000.0;

// Custom-painted Wii-style disc: convex plastic look with a glass top-half sheen,
// blue glow ring, inner white highlight, dark bottom shading, hover grow. The sheen
// (a white ellipse clipped to the top half) needs a real paint pass — QSS can't do it.
class WiiDiscButton : public QToolButton
{
public:
    WiiDiscButton(const QString& glyph, const QString& tip,
                  qreal baseDia = 64.0, bool enabled = true, QWidget* parent = nullptr)
        : QToolButton(parent), m_glyph(glyph), m_baseDia(baseDia)
    {
        setToolTip(tip);
        setEnabled(enabled);
        setCursor(enabled ? Qt::PointingHandCursor : Qt::ArrowCursor);
        const int box = int(baseDia + GLOW_PAD); // room for glow + 1.08x grow
        setFixedSize(box, box);

        if (enabled)
        {
            auto* sh = new QGraphicsDropShadowEffect(this);
            sh->setBlurRadius(12);
            sh->setOffset(0, 3);
            sh->setColor(QColor(0, 0, 0, 0x40)); // #40000000
            setGraphicsEffect(sh);
        }
    }

    static qreal glowPad() { return GLOW_PAD; }

protected:
    void enterEvent(QEnterEvent* e) override { m_hover = true;  update(); QToolButton::enterEvent(e); }
    void leaveEvent(QEvent* e)      override { m_hover = false; update(); QToolButton::leaveEvent(e); }

    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        const bool on   = isEnabled();
        const bool hov  = on && m_hover;
        const qreal dia = (hov ? m_baseDia * 1.08 : m_baseDia); // grow on hover
        QRectF disc((width() - dia) / 2.0, (height() - dia) / 2.0, dia, dia);
        const bool down = on && isDown();
        const qreal ringW = qMax(2.0, dia * 0.047);

        // Blue outer glow — a few fading strokes just outside the ring (enabled only).
        if (on)
        {
            const int glowA = hov ? 150 : 90;
            for (int i = 3; i >= 1; --i)
            {
                QColor c(0x7E, 0xC8, 0xE3, glowA / (i + 1));
                QPen gp(c); gp.setWidthF(3.0 + i * 2.0);
                p.setPen(gp); p.setBrush(Qt::NoBrush);
                p.drawEllipse(disc.adjusted(-i, -i, i, i));
            }
        }

        // Base fill: radial, lighter toward top-left, darker at the edge (light from TL).
        QRadialGradient base(disc.center() - QPointF(disc.width() * 0.14, disc.height() * 0.18),
                             disc.width() * 0.75);
        if (on)
        {
            base.setColorAt(0.0, QColor(0xEC, 0xF1, 0xF6));
            base.setColorAt(0.7, QColor(0xD0, 0xD8, 0xE0));
            base.setColorAt(1.0, QColor(0xB2, 0xBE, 0xCB));
        }
        else
        {
            base.setColorAt(0.0, QColor(0xF1, 0xF3, 0xF5));
            base.setColorAt(1.0, QColor(0xDD, 0xE2, 0xE6));
        }
        p.setPen(Qt::NoPen);
        p.setBrush(base);
        p.drawEllipse(disc);

        QPainterPath discPath; discPath.addEllipse(disc);

        // Bottom-half shading.
        p.save();
        p.setClipPath(discPath);
        QLinearGradient bsh(disc.topLeft(), disc.bottomLeft());
        bsh.setColorAt(0.5, QColor(0, 0, 0, 0));
        bsh.setColorAt(1.0, QColor(0, 0, 0, on ? (down ? 70 : 45) : 20));
        p.setBrush(bsh); p.setPen(Qt::NoPen);
        p.drawRect(disc);
        p.restore();

        // Glass sheen: bright reflection catch leaning to the TOP-LEFT (light source
        // from upper-left), clipped to the top half of the disc. ~70% more obvious.
        if (on && !down)
        {
            p.save();
            p.setClipPath(discPath);
            QRectF topHalf(disc.left(), disc.top(), disc.width(), disc.height() * 0.55);
            p.setClipRect(topHalf, Qt::IntersectClip);
            // Reflection ellipse shifted left + up; radial catch centred near its
            // top-left so the highlight reads as an angled light source.
            QRectF sheen = disc.adjusted(disc.width() * 0.07, disc.height() * 0.05,
                                         -disc.width() * 0.22, -disc.height() * 0.42);
            QPointF focus(sheen.left() + sheen.width() * 0.32,
                          sheen.top()  + sheen.height() * 0.28);
            QRadialGradient sg(focus, sheen.width() * 0.85, focus);
            const int peak = hov ? 255 : 250; // was ~150/205 → ~70% more visible
            sg.setColorAt(0.0, QColor(255, 255, 255, peak));
            sg.setColorAt(0.55, QColor(255, 255, 255, peak * 0.45));
            sg.setColorAt(1.0, QColor(255, 255, 255, 0));
            p.setBrush(sg); p.setPen(Qt::NoPen);
            p.drawEllipse(sheen);
            p.restore();
        }

        // Inner white highlight ring + outer ring.
        p.setBrush(Qt::NoBrush);
        if (on)
        {
            QPen white(QColor(255, 255, 255, 200)); white.setWidthF(1.0);
            p.setPen(white); p.drawEllipse(disc.adjusted(2.5, 2.5, -2.5, -2.5));
        }
        QColor ringC = on ? (hov ? QColor(0x9A, 0xD8, 0xF2) : QColor(0x7E, 0xC8, 0xE3))
                          : QColor(0xC2, 0xCC, 0xD4);
        QPen ring(ringC); ring.setWidthF(ringW);
        p.setPen(ring); p.drawEllipse(disc.adjusted(1.5, 1.5, -1.5, -1.5));

        // Glyph.
        p.setPen(on ? QColor(0x2A, 0x6F, 0xA0) : QColor(0x9A, 0xA7, 0xB2));
        QFont f = font(); f.setPixelSize(int(dia * 0.40)); f.setBold(true);
        p.setFont(f);
        p.drawText(disc.translated(0, down ? 1.5 : 0), Qt::AlignCenter, m_glyph);
    }

private:
    static constexpr qreal GLOW_PAD = 12.0; // widget padding around the disc
    QString m_glyph;
    qreal   m_baseDia;
    bool    m_hover = false;
};

// Convenience: small glass disc for the faceplate icon row.
WiiDiscButton* makeDiscButton(const QString& glyph, const QString& tip, bool enabled = true)
{
    return new WiiDiscButton(glyph, tip, 40.0, enabled);
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
    // Same glass-disc style as the bottom bar; box sized to ARROW_SIZE so the existing
    // placement math stays valid.
    auto makeArrow = [this](const QString& glyph, const QString& tip) {
        return new WiiDiscButton(glyph, tip, ARROW_SIZE - WiiDiscButton::glowPad(), true, this);
    };
    leftArrow = makeArrow(QString::fromUtf8("◀"), tr("Previous"));   // ◀
    rightArrow = makeArrow(QString::fromUtf8("▶"), tr("Next"));      // ▶
    connect(leftArrow, &QToolButton::clicked, carousel, &CoverCarousel::prev);
    connect(rightArrow, &QToolButton::clicked, carousel, &CoverCarousel::next);

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
    // Glassy faceplate: bright top sheen → translucent body, lit top edge.
    bar->setStyleSheet(
        "QFrame {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0    rgba(255,255,255,0.99),"
        "    stop:0.10 rgba(253,254,255,0.98),"
        "    stop:0.50 rgba(244,248,252,0.96),"
        "    stop:0.88 rgba(240,245,250,0.96),"
        "    stop:1    rgba(245,249,253,0.97));"
        "  border-top: 2px solid rgba(150,200,240,0.75);"
        "}"
        "QLabel { color: #3A4750; background: transparent; }");

    // Left Wii button → quick settings popup (glass disc style).
    auto* gearBtn = new WiiDiscButton(QString::fromUtf8("⚙"), tr("Quick settings"));
    connect(gearBtn, &QToolButton::clicked, this, [this]() { emit settingsRequested(); });

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

    // Faceplate icon row: sort + the folder actions (add / manage / rescan).
    auto* sortBtn   = makeDiscButton(QString::fromUtf8("⇅"), tr("Sort A–Z / Z–A"), true);
    auto* addBtn    = makeDiscButton(QString::fromUtf8("＋"), tr("Add Game…"), true);
    auto* savesBtn  = makeDiscButton(QString::fromUtf8("\U0001F4BE"), tr("Saves…"), true); // 💾
    auto* manageBtn = makeDiscButton(QString::fromUtf8("\U0001F4C1"), tr("Manage Folders…"), true);
    auto* rescanBtn = makeDiscButton(QString::fromUtf8("⟳"), tr("Rescan"), true);
    connect(sortBtn,   &QToolButton::clicked, this, &GameLibraryWidget::onSortToggled);
    connect(addBtn,    &QToolButton::clicked, this, &GameLibraryWidget::onAddGame);
    connect(savesBtn,  &QToolButton::clicked, this, [this]() { emit pathSettingsRequested(); });
    connect(manageBtn, &QToolButton::clicked, this, &GameLibraryWidget::onManageFolders);
    connect(rescanBtn, &QToolButton::clicked, this, &GameLibraryWidget::onRescan);

    auto* iconRow = new QHBoxLayout();
    iconRow->setSpacing(12);
    iconRow->addWidget(sortBtn);
    iconRow->addWidget(addBtn);
    iconRow->addWidget(savesBtn);   // right of Add Game
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
    auto* startBtn = new WiiDiscButton(QString::fromUtf8("▶"), tr("Start game"), 64.0); // ▶
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

void GameLibraryWidget::onAddGame()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Add Game"), QString(),
        tr("NDS ROMs (*.nds *.dsi *.srl *.ids *.nds.zst *.dsi.zst *.srl.zst);;All files (*.*)"));
    if (files.isEmpty()) return;

    // Library scans folders, so add each picked ROM's containing folder (deduped).
    QStringList dirs;
    for (const QString& f : files)
    {
        const QString dir = QFileInfo(f).absolutePath();
        if (!dir.isEmpty() && !dirs.contains(dir))
            dirs.append(dir);
    }
    addFolders(dirs);
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
    seenGames.clear();
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
    // Same game can sit in two scanned folders; show it once. Key by gameCode,
    // falling back to the ROM path for homebrew with no/blank code.
    const QString key = entry.gameCode.isEmpty() ? entry.romPath : entry.gameCode;
    if (seenGames.contains(key)) return;
    seenGames.insert(key);

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

// ----------------------------------------------------------- gamepad navigation

void GameLibraryWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    openGamepad();
    setFocus(); // so keyboard nav works immediately too
}

void GameLibraryWidget::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    closeGamepad(); // never poll the pad while a game is running
}

void GameLibraryWidget::openGamepad()
{
    if (!padTimer)
    {
        padTimer = new QTimer(this);
        padTimer->setInterval(PAD_POLL_MS);
        connect(padTimer, &QTimer::timeout, this, &GameLibraryWidget::onPollGamepad);
    }
    // SDL is already initialised by the app; make sure the gamecontroller subsystem is up.
    if (!SDL_WasInit(SDL_INIT_GAMECONTROLLER))
        SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);

    padLastDir = 0;
    padRepeatCountdown = 0;
    padAHeld = false;
    padTimer->start();
}

void GameLibraryWidget::closeGamepad()
{
    if (padTimer) padTimer->stop();
    if (pad)
    {
        SDL_GameControllerClose(pad);
        pad = nullptr;
    }
}

void GameLibraryWidget::onPollGamepad()
{
    SDL_GameControllerUpdate();

    // Lazily grab the first attached controller; reacquire if it was unplugged.
    if (!pad || !SDL_GameControllerGetAttached(pad))
    {
        if (pad) { SDL_GameControllerClose(pad); pad = nullptr; }
        const int n = SDL_NumJoysticks();
        for (int i = 0; i < n; i++)
        {
            if (SDL_IsGameController(i)) { pad = SDL_GameControllerOpen(i); break; }
        }
        if (!pad) return;
    }

    // Direction from D-pad or left-stick X (with deadzone).
    int dir = 0;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) dir = +1;
    else if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_LEFT)) dir = -1;
    else
    {
        const Sint16 ax = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX);
        if (ax >  PAD_AXIS_DEADZONE) dir = +1;
        else if (ax < -PAD_AXIS_DEADZONE) dir = -1;
    }

    if (dir == 0)
    {
        padLastDir = 0;
        padRepeatCountdown = 0;
    }
    else if (dir != padLastDir)
    {
        // New press → act now, then wait the initial delay before repeating.
        (dir > 0) ? carousel->next() : carousel->prev();
        padLastDir = dir;
        padRepeatCountdown = PAD_INITIAL_TICKS;
    }
    else if (--padRepeatCountdown <= 0)
    {
        (dir > 0) ? carousel->next() : carousel->prev();
        padRepeatCountdown = PAD_REPEAT_TICKS;
    }

    // A button → boot the centred game (edge-triggered).
    const bool aNow = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_A);
    if (aNow && !padAHeld && carousel->count() > 0)
        onCoverActivated(carousel->currentIndex());
    padAHeld = aNow;
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

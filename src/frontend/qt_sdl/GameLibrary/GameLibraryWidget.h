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

#pragma once

#include <QWidget>
#include <QStringList>
#include <QSet>
#include "GameEntry.h"

class QThread;
class QLabel;
class QPushButton;
class QToolButton;
class QFrame;
class QTimer;
class GameLibraryModel;
class CoverCarousel;
class RomScanner;
class CoverFetcher;

// The game-selection screen: a USB Loader GX-style fan carousel of cover art over
// a Wii-faceplate bottom bar. Scans ROM folders on a worker thread and boots a
// game when one is activated.
class GameLibraryWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GameLibraryWidget(QWidget* parent = nullptr);
    ~GameLibraryWidget() override;

    // Replace the scanned folders and rescan. Does not emit foldersChanged.
    void setFolders(const QStringList& folders);
    QStringList folders() const { return scanFolders; }

signals:
    void gameActivated(const QString& romPath);
    void foldersChanged(const QStringList& folders); // user added/removed a folder
    void settingsRequested();                        // gear button → quick settings
    void pathSettingsRequested();                    // saves button → save/savestate path settings

public slots:
    void onAddFolder();
    void onAddGame(); // pick individual ROM file(s); adds their containing folder(s)

private slots:
    void onManageFolders();
    void onRescan();
    void onCoverActivated(int row);
    void onSortToggled();
    void onEntryFound(const GameEntry& entry);
    void onScanFinished(int count);
    void onPollGamepad(); // drive the carousel with a controller while visible

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void startScan();
    void addFolders(const QStringList& newFolders); // dedupe, persist, rescan
    QFrame* buildBottomBar();
    void positionArrows();
    void refreshStorageLine(int count);

    GameLibraryModel* model;
    CoverCarousel* carousel;
    QToolButton* leftArrow;
    QToolButton* rightArrow;
    QLabel* clockLabel;
    QLabel* storageLabel;
    QTimer* clock;
    bool sortAscending;

    RomScanner* scanner;
    QThread* scanThread;
    CoverFetcher* coverFetcher;

    QStringList scanFolders;
    QSet<QString> seenGames; // dedupe key per scan (gameCode, or path if code empty)

    // Gamepad navigation of the carousel (active only while this screen is shown).
    void openGamepad();
    void closeGamepad();
    QTimer* padTimer = nullptr;
    struct _SDL_GameController* pad = nullptr;
    int  padLastDir = 0;       // -1/0/+1 for held direction (auto-repeat)
    int  padRepeatCountdown = 0;
    bool padAHeld = false;     // edge-detect the boot button
};

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
#include "GameEntry.h"

class QListView;
class QThread;
class QLabel;
class QLineEdit;
class QSortFilterProxyModel;
class GameLibraryModel;
class RomScanner;
class CoverFetcher;

// The game-selection screen: a grid of cover art. Scans ROM folders on a worker
// thread and boots a game when one is activated.
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

public slots:
    void onAddFolder();

private slots:
    void onManageFolders();
    void onRescan();
    void onItemActivated(const QModelIndex& index);
    void onEntryFound(const GameEntry& entry);
    void onScanFinished(int count);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void startScan();
    void updateEmptyState(int count);
    void addFolders(const QStringList& newFolders); // dedupe, persist, rescan

    QListView* view;
    GameLibraryModel* model;
    QSortFilterProxyModel* proxy;
    QLineEdit* searchBox;
    QLabel* statusLabel;

    RomScanner* scanner;
    QThread* scanThread;
    CoverFetcher* coverFetcher;

    QStringList scanFolders;
};

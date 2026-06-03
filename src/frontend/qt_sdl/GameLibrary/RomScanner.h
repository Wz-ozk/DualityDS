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

#include <QObject>
#include <QStringList>
#include "GameEntry.h"

// Scans a set of folders for NDS ROMs and reads each one's metadata.
// Designed to live on a worker QThread: call scan() via a queued connection,
// listen for entryFound()/finished() on the GUI thread.
class RomScanner : public QObject
{
    Q_OBJECT
public:
    explicit RomScanner(QObject* parent = nullptr);

    // Call once at startup to make GameEntry usable across thread boundaries.
    static void registerTypes();

public slots:
    void scan(const QStringList& folders);

signals:
    void entryFound(const GameEntry& entry);
    void finished(int count);
};

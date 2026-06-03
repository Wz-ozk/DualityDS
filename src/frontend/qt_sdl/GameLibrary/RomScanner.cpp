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

#include "RomScanner.h"
#include "GameMetadata.h"

#include <QDirIterator>
#include <QSet>

RomScanner::RomScanner(QObject* parent) : QObject(parent)
{
}

void RomScanner::registerTypes()
{
    qRegisterMetaType<GameEntry>("GameEntry");
}

void RomScanner::scan(const QStringList& folders)
{
    static const QStringList filters = {
        "*.nds", "*.dsi", "*.srl", "*.ids",
        "*.nds.zst", "*.dsi.zst", "*.srl.zst"
    };

    QSet<QString> seen; // dedupe by canonical path across overlapping folders
    int count = 0;

    for (const QString& folder : folders)
    {
        QDirIterator it(folder, filters, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext())
        {
            QString path = it.next();
            QString canonical = QFileInfo(path).canonicalFilePath();
            if (canonical.isEmpty()) canonical = path;
            if (seen.contains(canonical)) continue;
            seen.insert(canonical);

            GameEntry entry;
            if (GameMetadata::read(canonical, entry))
            {
                emit entryFound(entry);
                count++;
            }
        }
    }

    emit finished(count);
}

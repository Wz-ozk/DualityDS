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

#include <QString>
#include <QImage>
#include <QMetaType>

// One game in the library grid. Metadata (gameCode/title/bannerIcon) comes from
// the ROM header itself; coverPath is filled later by the online cover fetcher.
struct GameEntry
{
    QString romPath;     // full path to the .nds file
    QString gameCode;    // 4-char NDS game code, e.g. "CPUE" (Platinum US)
    QString title;       // human-readable title (banner or internal GameTitle)
    QString coverPath;   // local cached cover image, empty until fetched
    QImage  bannerIcon;  // 32x32 icon decoded from the ROM banner (offline fallback)
};

Q_DECLARE_METATYPE(GameEntry)

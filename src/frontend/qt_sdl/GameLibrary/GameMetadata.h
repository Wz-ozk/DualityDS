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

#include "GameEntry.h"

// Reads NDS ROM header + banner without loading the whole cartridge.
// Pulls game code, a display title, and decodes the 32x32 banner icon.
namespace GameMetadata
{
    // Fills romPath/gameCode/title/bannerIcon on `out`. Returns false if the
    // file can't be opened or isn't a plausible NDS ROM. Even on a partial read
    // (e.g. missing banner) it fills what it can and returns true.
    bool read(const QString& romPath, GameEntry& out);
}

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

#include "GameMetadata.h"

#include <QFile>
#include <QFileInfo>
#include <cstdint>

namespace GameMetadata
{

// NDS header field offsets (GBATEK)
static const int HDR_GAME_TITLE   = 0x000; // 12 bytes ASCII
static const int HDR_GAME_CODE    = 0x00C; // 4 bytes ASCII
static const int HDR_BANNER_OFFSET= 0x068; // u32 LE: offset of banner in ROM

// NDSBanner field offsets, relative to the banner start
static const int BNR_ICON         = 0x020; // 512 bytes, 32x32 4bpp tiled
static const int BNR_PALETTE      = 0x220; // 16 entries, BGR555
static const int BNR_TITLE_EN     = 0x340; // EnglishTitle, 128 char16_t UTF-16LE

static uint32_t readU32LE(const QByteArray& b, int off)
{
    const uchar* p = reinterpret_cast<const uchar*>(b.constData()) + off;
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

// BGR555 (15-bit) -> 0xAARRGGBB. Index 0 is transparent by NDS convention.
static QRgb bgr555ToRgb(uint16_t c, bool transparent)
{
    int r = (c & 0x1F) << 3;
    int g = ((c >> 5) & 0x1F) << 3;
    int b = ((c >> 10) & 0x1F) << 3;
    r |= r >> 5; g |= g >> 5; b |= b >> 5; // expand 5->8 bits
    return transparent ? qRgba(0, 0, 0, 0) : qRgba(r, g, b, 255);
}

// Decode the 32x32 banner icon: 4x4 grid of 8x8 tiles, 4 bits per pixel.
static QImage decodeIcon(const QByteArray& icon, const QByteArray& pal)
{
    if (icon.size() < 512 || pal.size() < 32)
        return QImage();

    uint16_t palette[16];
    for (int i = 0; i < 16; i++)
    {
        const uchar* p = reinterpret_cast<const uchar*>(pal.constData()) + i * 2;
        palette[i] = (uint16_t)p[0] | ((uint16_t)p[1] << 8);
    }

    QImage img(32, 32, QImage::Format_ARGB32);
    const uchar* data = reinterpret_cast<const uchar*>(icon.constData());

    for (int tileY = 0; tileY < 4; tileY++)
    for (int tileX = 0; tileX < 4; tileX++)
    {
        int tileIdx = tileY * 4 + tileX;
        for (int py = 0; py < 8; py++)
        for (int px = 0; px < 8; px++)
        {
            int byteIdx = tileIdx * 32 + py * 4 + (px >> 1);
            uchar byte = data[byteIdx];
            int idx = (px & 1) ? (byte >> 4) : (byte & 0x0F);
            img.setPixel(tileX * 8 + px, tileY * 8 + py,
                         bgr555ToRgb(palette[idx], idx == 0));
        }
    }

    return img;
}

static QString decodeTitle(const QByteArray& raw)
{
    // UTF-16LE, up to 128 code units, null-terminated, newlines separate lines.
    QString s = QString::fromUtf16(reinterpret_cast<const char16_t*>(raw.constData()),
                                   raw.size() / 2);
    int nul = s.indexOf(QChar(0));
    if (nul >= 0) s.truncate(nul);
    s.replace('\n', ' ');
    return s.trimmed();
}

bool read(const QString& romPath, GameEntry& out)
{
    QFile f(romPath);
    if (!f.open(QIODevice::ReadOnly))
        return false;

    QByteArray header = f.read(0x200);
    if (header.size() < 0x200)
    {
        f.close();
        return false;
    }

    out.romPath = romPath;

    out.gameCode = QString::fromLatin1(header.constData() + HDR_GAME_CODE, 4).trimmed();

    QString internalTitle =
        QString::fromLatin1(header.constData() + HDR_GAME_TITLE, 12).trimmed();

    // Banner: decode icon + grab the English title if the offset looks sane.
    uint32_t bannerOff = readU32LE(header, HDR_BANNER_OFFSET);
    QString bannerTitle;
    if (bannerOff != 0 && bannerOff < (uint32_t)f.size())
    {
        if (f.seek(bannerOff + BNR_ICON))
        {
            QByteArray icon = f.read(512);
            f.seek(bannerOff + BNR_PALETTE);
            QByteArray pal = f.read(32);
            out.bannerIcon = decodeIcon(icon, pal);
        }
        if (f.seek(bannerOff + BNR_TITLE_EN))
            bannerTitle = decodeTitle(f.read(256));
    }

    if (!bannerTitle.isEmpty())
        out.title = bannerTitle;
    else if (!internalTitle.isEmpty())
        out.title = internalTitle;
    else
        out.title = QFileInfo(romPath).completeBaseName();

    f.close();
    return true;
}

} // namespace GameMetadata

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

#include "GameLibraryModel.h"

#include <QPixmap>
#include <algorithm>

GameLibraryModel::GameLibraryModel(QObject* parent) : QAbstractListModel(parent)
{
}

int GameLibraryModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return entries.size();
}

QVariant GameLibraryModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= entries.size())
        return QVariant();

    const GameEntry& e = entries.at(index.row());

    switch (role)
    {
    case Qt::DisplayRole:
        return e.title;

    case Qt::DecorationRole:
        // Prefer a fetched cover; fall back to the ROM banner icon, scaled up
        // with no smoothing to keep the pixel art crisp.
        if (!e.coverPath.isEmpty())
        {
            QPixmap cover(e.coverPath);
            if (!cover.isNull())
                return cover.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        if (!e.bannerIcon.isNull())
            return QPixmap::fromImage(e.bannerIcon)
                .scaled(128, 128, Qt::KeepAspectRatio, Qt::FastTransformation);
        return QVariant();

    case Qt::ToolTipRole:
        return QString("%1\n%2").arg(e.title, e.gameCode);

    case CoverPixmapRole:
        // Full-resolution cover for the carousel (DecorationRole hard-scales to
        // 128px, too small for the large center cover). Prefer the fetched cover;
        // fall back to the ROM banner icon upscaled with no smoothing.
        if (!e.coverPath.isEmpty())
        {
            QPixmap cover(e.coverPath);
            if (!cover.isNull())
                return cover;
        }
        if (!e.bannerIcon.isNull())
            return QPixmap::fromImage(e.bannerIcon);
        return QVariant();

    case RomPathRole:
        return e.romPath;

    case GameCodeRole:
        return e.gameCode;
    }

    return QVariant();
}

void GameLibraryModel::clear()
{
    if (entries.isEmpty()) return;
    beginResetModel();
    entries.clear();
    endResetModel();
}

void GameLibraryModel::addEntry(const GameEntry& entry)
{
    beginInsertRows(QModelIndex(), entries.size(), entries.size());
    entries.append(entry);
    endInsertRows();
}

void GameLibraryModel::sortByTitle(bool ascending)
{
    if (entries.size() < 2) return;
    beginResetModel();
    std::sort(entries.begin(), entries.end(),
              [ascending](const GameEntry& a, const GameEntry& b) {
                  int c = a.title.compare(b.title, Qt::CaseInsensitive);
                  return ascending ? (c < 0) : (c > 0);
              });
    endResetModel();
}

void GameLibraryModel::setCover(int row, const QString& coverPath)
{
    if (row < 0 || row >= entries.size()) return;
    entries[row].coverPath = coverPath;
    QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {Qt::DecorationRole, CoverPixmapRole});
}

QString GameLibraryModel::romPathAt(const QModelIndex& index) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= entries.size())
        return QString();
    return entries.at(index.row()).romPath;
}

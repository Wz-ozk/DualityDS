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

#include <QAbstractListModel>
#include <QList>
#include "GameEntry.h"

class GameLibraryModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles { RomPathRole = Qt::UserRole + 1, GameCodeRole, CoverPixmapRole };

    explicit GameLibraryModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    void clear();
    void addEntry(const GameEntry& entry);
    void sortByTitle(bool ascending); // reorders entries (covers travel with them)
    void setCover(int row, const QString& coverPath);
    QString romPathAt(const QModelIndex& index) const;

private:
    QList<GameEntry> entries;
};

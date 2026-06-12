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
#include <QString>
#include <QStringList>
#include <QHash>

class QNetworkAccessManager;
class QNetworkReply;

// Fetches box art from GameTDB by NDS game code, caching each cover to disk so
// it is downloaded at most once. Lives on the GUI thread (QNAM is async).
// On miss/offline the caller keeps showing the ROM's banner icon.
class CoverFetcher : public QObject
{
    Q_OBJECT
public:
    explicit CoverFetcher(const QString& cacheDir, QObject* parent = nullptr);

    // Request the cover for `gameCode`, reporting back against `index`. `romName`
    // (ROM file basename) and `title` (banner title) are used to look up higher-
    // quality libretro boxart before falling back to GameTDB. Emits coverReady
    // immediately if already cached.
    void request(int index, const QString& gameCode,
                 const QString& romName, const QString& title);

    // Abort all in-flight downloads (e.g. before a rescan invalidates indices).
    void cancelAll();

signals:
    void coverReady(int index, const QString& path);

private slots:
    void onFinished(QNetworkReply* reply);

private:
    struct Pending { int index; QString gameCode; QStringList urls; int attempt; };

    void startNext(QNetworkReply* prev, Pending p);
    QString cachePath(const QString& gameCode) const;
    static QStringList buildUrls(const QString& gameCode,
                                 const QString& romName, const QString& title);

    QNetworkAccessManager* nam;
    QString cacheDir;
    QHash<QNetworkReply*, Pending> pending;
};

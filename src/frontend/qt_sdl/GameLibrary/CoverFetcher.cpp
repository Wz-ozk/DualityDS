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

#include "CoverFetcher.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QImage>
#include <QUrl>

CoverFetcher::CoverFetcher(const QString& cacheDir, QObject* parent)
    : QObject(parent), cacheDir(cacheDir)
{
    QDir().mkpath(cacheDir);
    nam = new QNetworkAccessManager(this);
    connect(nam, &QNetworkAccessManager::finished, this, &CoverFetcher::onFinished);
}

QString CoverFetcher::cachePath(const QString& gameCode) const
{
    return cacheDir + "/" + gameCode + ".png";
}

// GameTDB stores art under language/region folders. Pick likely folders from
// the 4th game-code character (region), always falling back to EN/US.
QStringList CoverFetcher::buildUrls(const QString& gameCode)
{
    QChar region = gameCode.size() >= 4 ? gameCode.at(3) : QChar('E');

    QStringList folders;
    switch (region.toLatin1())
    {
    case 'E': case 'T': folders = {"US", "EN"}; break;          // Americas
    case 'J':           folders = {"JA"}; break;                // Japan
    case 'K':           folders = {"KO"}; break;                // Korea
    case 'C':           folders = {"ZHCN"}; break;              // China
    case 'P': case 'X': case 'Y': case 'Z': case 'D':
    case 'F': case 'S': case 'I': case 'H':
        folders = {"EN"}; break;                                // Europe
    default:            folders = {"EN", "US"}; break;
    }
    // generic fallbacks
    for (const QString& f : {QString("EN"), QString("US"), QString("JA")})
        if (!folders.contains(f)) folders.append(f);

    QStringList urls;
    for (const QString& f : folders)
        urls.append(QString("https://art.gametdb.com/ds/coverS/%1/%2.png").arg(f, gameCode));
    return urls;
}

void CoverFetcher::request(int index, const QString& gameCode)
{
    if (gameCode.size() < 4)
        return;

    QString cached = cachePath(gameCode);
    if (QFileInfo::exists(cached))
    {
        emit coverReady(index, cached);
        return;
    }

    Pending p { index, gameCode, buildUrls(gameCode), 0 };
    startNext(nullptr, p);
}

void CoverFetcher::startNext(QNetworkReply* prev, Pending p)
{
    if (prev)
    {
        pending.remove(prev);
        prev->deleteLater();
    }

    if (p.attempt >= p.urls.size())
        return; // exhausted: caller keeps the banner-icon fallback

    QNetworkRequest req{QUrl(p.urls.at(p.attempt))};
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setHeader(QNetworkRequest::UserAgentHeader, "DualityDS");

    QNetworkReply* reply = nam->get(req);
    pending.insert(reply, p);
}

void CoverFetcher::onFinished(QNetworkReply* reply)
{
    auto it = pending.find(reply);
    if (it == pending.end())
    {
        reply->deleteLater();
        return;
    }
    Pending p = it.value();

    bool ok = false;
    if (reply->error() == QNetworkReply::NoError)
    {
        QByteArray data = reply->readAll();
        // Validate it actually decodes as an image (GameTDB serves a small
        // placeholder/404 page otherwise).
        QImage img;
        if (!data.isEmpty() && img.loadFromData(data))
        {
            QFile f(cachePath(p.gameCode));
            if (f.open(QIODevice::WriteOnly))
            {
                img.save(&f, "PNG");
                f.close();
                ok = true;
            }
        }
    }

    if (ok)
    {
        pending.remove(reply);
        reply->deleteLater();
        emit coverReady(p.index, cachePath(p.gameCode));
    }
    else
    {
        // try the next region/folder
        p.attempt++;
        startNext(reply, p);
    }
}

void CoverFetcher::cancelAll()
{
    const auto replies = pending.keys();
    for (QNetworkReply* r : replies)
    {
        r->abort();
        r->deleteLater();
    }
    pending.clear();
}

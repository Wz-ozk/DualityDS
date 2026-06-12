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
    // "_hq" suffix so any old low-res coverS cache is ignored and refetched.
    return cacheDir + "/" + gameCode + "_hq.png";
}

namespace
{
// libretro thumbnail filenames replace these characters with '_'.
QString sanitizeLibretro(QString s)
{
    static const QString bad = "&*/:`<>?\\|";
    for (QChar& c : s)
        if (bad.contains(c)) c = QChar('_');
    return s.trimmed();
}

// Strip diacritics so a banner title like "Pokémon" can match No-Intro "Pokemon".
QString foldAccents(const QString& s)
{
    QString d = s.normalized(QString::NormalizationForm_KD);
    QString out;
    out.reserve(d.size());
    for (QChar c : d)
        if (c.category() != QChar::Mark_NonSpacing)
            out.append(c);
    return out;
}

// No-Intro region suffix from the game-code region character.
QString regionSuffix(const QString& gameCode)
{
    QChar r = gameCode.size() >= 4 ? gameCode.at(3) : QChar('E');
    switch (r.toLatin1())
    {
    case 'E': case 'T': return "(USA)";
    case 'J':           return "(Japan)";
    case 'K':           return "(Korea)";
    case 'C':           return "(China)";
    case 'P': case 'X': case 'Y': case 'Z': case 'D':
    case 'F': case 'S': case 'I': case 'H': return "(Europe)";
    default:            return QString();
    }
}

QString libretroUrl(const QString& name)
{
    QUrl u;
    u.setScheme("https");
    u.setHost("thumbnails.libretro.com");
    u.setPath("/Nintendo - Nintendo DS/Named_Boxarts/" + name + ".png");
    return u.toString(QUrl::FullyEncoded);
}
} // namespace

// Builds the ordered list of cover URLs to try: higher-quality libretro boxart
// first (keyed by game name), then GameTDB art (keyed by game code).
QStringList CoverFetcher::buildUrls(const QString& gameCode,
                                    const QString& romName, const QString& title)
{
    QStringList urls;

    // --- libretro Named_Boxarts (highest quality), keyed by the game name ---
    QStringList names;
    auto pushName = [&](const QString& n) {
        QString s = sanitizeLibretro(n);
        if (!s.isEmpty() && !names.contains(s)) names.append(s);
    };
    pushName(romName);                       // ROM basename, often No-Intro named
    const QString folded = foldAccents(title).trimmed();
    const QString suffix = regionSuffix(gameCode);
    if (!folded.isEmpty() && !suffix.isEmpty())
        pushName(folded + " " + suffix);     // "Title (Region)"
    pushName(folded);                        // bare title, last resort
    for (const QString& n : names)
        urls.append(libretroUrl(n));

    // --- GameTDB fallback ---
    // Pick likely regions from the 4th game-code character, and prefer the high-res
    // art sets (coverHQ > cover > coverS). Always fall back to EN/US.
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

    // Try high-res sets first; coverS is the last-resort small art.
    static const QStringList types = {"coverHQ", "cover", "coverS"};

    for (const QString& t : types)
        for (const QString& f : folders)
            urls.append(QString("https://art.gametdb.com/ds/%1/%2/%3.png").arg(t, f, gameCode));
    return urls;
}

void CoverFetcher::request(int index, const QString& gameCode,
                           const QString& romName, const QString& title)
{
    if (gameCode.size() < 4)
        return;

    QString cached = cachePath(gameCode);
    if (QFileInfo::exists(cached))
    {
        emit coverReady(index, cached);
        return;
    }

    Pending p { index, gameCode, buildUrls(gameCode, romName, title), 0 };
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

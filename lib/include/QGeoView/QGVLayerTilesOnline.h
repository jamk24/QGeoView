/***************************************************************************
 * QGeoView is a Qt / C ++ widget for visualizing geographic data.
 * Copyright (C) 2018-2024 Andrey Yaroshenko.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see https://www.gnu.org/licenses.
 ****************************************************************************/

#pragma once

#include "QGVLayerTiles.h"
#include "QGVLayerTilesOnlineCache.h"

#include <QNetworkReply>
#include <QIODevice>
#include <QFile>
#include <QImage>
#include <QPen>
#include <QPainter>

class QGV_LIB_DECL QGVLayerTilesOnline : public QGVLayerTiles
{
    Q_OBJECT

public:
    QGVLayerTilesOnline();
    ~QGVLayerTilesOnline();
    void setCache(bool mode);
    void setOffline(bool mode);
    int loadTilesFromGeo(QGV::GeoRect areaGeoRect, int zoom);

protected:
    virtual QString tilePosToUrl(const QGV::GeoTilePos& tilePos) const = 0;

private:
    void request(const QGV::GeoTilePos& tilePos) override;
    void cancel(const QGV::GeoTilePos& tilePos) override;
    void onReplyFinished(QNetworkReply* reply, const QGV::GeoTilePos& tilePos);
    void removeReply(const QGV::GeoTilePos& tilePos);
    void incOfflineCnt();
private:
    QMap<QGV::GeoTilePos, QNetworkReply*> mRequest;
    QGVLayerTilesOnlineCache mCache;
    int offline_counter = 0;
    int offline_cnt_max = 50;
    bool isOffline = false;
    bool isCache = true;
};

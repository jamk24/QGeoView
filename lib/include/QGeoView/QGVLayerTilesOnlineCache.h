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
#include <sqlite3.h>

#include <QNetworkReply>
#include <QIODevice>
#include <QFile>
#include <QImage>
#include <QPen>
#include <QPainter>
#include <QDir>

#define cache_dir     "cache/"
#define cache_db      "cache.db"
#define d_width       256
#define d_height      256
#define cache_insert  "insert into tiles_cache (t_scheme,t_x,t_y,t_zoom,t_name,t_datetime,t_datetime_u,t_size) values('%1',%2,%3,%4,'%5',datetime(),strftime('%s', 'now'),%6);"
#define cache_sel     "select t_x,t_y,t_zoom,t_name,t_datetime,t_datetime_u from tiles_cache where t_scheme = '%1' and t_x = %2 and t_y = %3 and t_zoom = %4;"
#define cache_create  "CREATE TABLE IF NOT EXISTS tiles_cache(t_scheme text NOT NULL,	t_x integer NOT NULL, t_y integer NOT NULL,	t_zoom integer NOT NULL, t_name text, t_datetime text NOT NULL,	t_datetime_u integer NOT NULL, t_type text,	t_size int, PRIMARY KEY(t_scheme, t_x, t_y, t_zoom));"
#define db_col_name   3

class QGV_LIB_DECL QGVLayerTilesOnlineCache
{
public:
    ~QGVLayerTilesOnlineCache();
    void init_cache();
    QByteArray getTileFromCache(const QGV::GeoTilePos& tilePos, QString tile_name, QString prv_name);
    QImage getNoData(QString _text);
    bool putTileToCache(const QGV::GeoTilePos& tilePos, QString tile_name, QString prv_name, QByteArray raw_tile);

protected:
    

private:
    QString parseUrl2fileName(QString url);
    int insertTile2Db(const QGV::GeoTilePos& tilePos, QString tile_fname, QString prv_name, int fsize);
    bool createCache2Db();
    QString getTileFromDb(const QGV::GeoTilePos& tilePos, QString prv_name);

    sqlite3 *mDb;
    int mret;
    int cache_time = 3600;
};

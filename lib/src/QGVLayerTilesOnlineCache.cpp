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

#include "QGVLayerTilesOnlineCache.h"
#include "Raster/QGVImage.h"


namespace {


}

void QGVLayerTilesOnlineCache::init_cache()
{
    QDir().mkdir(cache_dir);

    // SQLITE_OPEN_CREATE
    mret = sqlite3_open(cache_db, &mDb);
    if (mret)
    {
        qgvDebug() << "Can't open database: " << sqlite3_errmsg(mDb);
    }
    else
    {
        qgvDebug() << "Open database successfully\n";
        createCache2Db();
    }
}


QGVLayerTilesOnlineCache::~QGVLayerTilesOnlineCache()
{
    //qDeleteAll(mRequest);
    qgvDebug() << "close database..";
    sqlite3_close(mDb);
}


QString QGVLayerTilesOnlineCache::parseUrl2fileName(QString url)
{
    QString temp_str;

    temp_str = url.remove(QString("http://"), Qt::CaseSensitivity::CaseInsensitive);
    temp_str.replace(QChar('/'), QChar('_'));

    return temp_str;
}

QByteArray QGVLayerTilesOnlineCache::getTileFromCache(const QGV::GeoTilePos& tilePos, QString tile_name, QString prv_name)
{
    QString fname = parseUrl2fileName(tile_name);
    QFile cfile(QString(cache_dir) + fname);
    QByteArray rawImage;

    QString cache_fname = getTileFromDb(tilePos, prv_name);

    if (cache_fname.length())
    {
        qgvDebug() << "getTileFromCache: Db fname - " << cache_fname;
        fname = cache_fname;
    }
    else
    {
        qgvDebug() << "fname: " << fname;
    }

    // check if file exists in file cache
    if (cfile.exists())
    {
        qgvDebug() << "file in cache: " << fname;

        // load it from cache
        cfile.open(QIODevice::ReadOnly);

        rawImage = cfile.readAll();

        cfile.close();
    }

    return rawImage;
}

bool QGVLayerTilesOnlineCache::putTileToCache(const QGV::GeoTilePos& tilePos, QString tile_name, QString prv_name, QByteArray raw_tile)
{
    // save to file
    QString fname = parseUrl2fileName(tile_name);
    qgvDebug() << "putTileToCache: " << fname;
    QFile cfile(QString(cache_dir) + fname);
    if (cfile.open(QIODevice::WriteOnly))
    {
        cfile.write(raw_tile);
        cfile.close();

        // insert to database
        insertTile2Db(tilePos, fname, prv_name, raw_tile.length());

        return true;
    }

    return false;
}

QImage QGVLayerTilesOnlineCache::getNoData(QString _text)
{
    // draw
    QImage image(d_width, d_height, QImage::Format_ARGB32_Premultiplied);
    QPainter p;
    image.fill(Qt::black);

    if (p.begin(&image))
    {
        p.setPen(QPen(Qt::red));
        p.setFont(QFont("Times", 12, QFont::Bold));
        p.drawText(image.rect(), Qt::AlignCenter, _text);
        p.end();
    }

    return image;
}

bool QGVLayerTilesOnlineCache::createCache2Db()
{
    const char *pSQL;
    char *zErrMsg = 0;
    int rc;

    if (!mret)
    {
        QString ins_str = QString(cache_create);

        std::string str = ins_str.toStdString();

        pSQL = str.c_str();
        if (pSQL)
        {
            rc = sqlite3_exec(mDb, pSQL, NULL, 0, &zErrMsg);
            if (rc != SQLITE_OK)
            {
                qgvDebug() << "createCache2Db: create error";
                sqlite3_free(zErrMsg);
            }
        }
        else
        {
                qgvDebug() << "createCache2Db: query is empty!";
        }
        return true;
    }
    else
    {
        qgvDebug() << "createCache2Db: db connection not initialized!";
    }

    return false;
}
int QGVLayerTilesOnlineCache::insertTile2Db(const QGV::GeoTilePos& tilePos, QString tile_fname, QString prv_name, int fsize)
{
    const char *pSQL;
    char *zErrMsg = 0;
    int rc;

    if (!mret)
    {
        QString ins_str = QString(cache_insert).arg(prv_name).arg(tilePos.pos().x()).arg(tilePos.pos().y()).arg(tilePos.zoom()).arg(tile_fname).arg(fsize);

        std::string str = ins_str.toStdString();
        qgvDebug() << "insertTile2Db: " << ins_str;
        pSQL = str.c_str();
        if (pSQL)
        {
            rc = sqlite3_exec(mDb, pSQL, NULL, 0, &zErrMsg);
            if (rc != SQLITE_OK)
            {
                //cout<<"SQL error: "<<sqlite3_errmsg(db)<<"\n";
                qgvDebug() << "insertTile2Db: insert error";
                sqlite3_free(zErrMsg);
            }
        }
        else
        {
            qgvDebug() << "insertTile2Db: query is empty!";
        }
    }
    else
    {
        qgvDebug() << "insertTile2Db: db connection not initialized!";
    }

    return 0;
}

QString QGVLayerTilesOnlineCache::getTileFromDb(const QGV::GeoTilePos& tilePos, QString prv_name)
{
    const char *pSQL;
    char *zErrMsg = 0;
    QString tt_name;
    int rc;
    sqlite3_stmt *stmt;

    if (!mret)
    {
        //select t_x,t_y,t_zoom,t_name,t_datetime,t_datetime_u from tiles_cache where t_scheme = '%1' and t_x = %2 and t_y = %3 and t_zoom = %4;
        QString sel_str = QString(cache_sel).arg(prv_name).arg(tilePos.pos().x()).arg(tilePos.pos().y()).arg(tilePos.zoom());

        std::string str = sel_str.toStdString();
        qgvDebug() << "getTileFromDb: " << sel_str;

        pSQL = str.c_str();
        if (pSQL)
        {
            rc = sqlite3_exec(mDb, pSQL, NULL, 0, &zErrMsg);
            if (rc != SQLITE_OK)
            {
                qgvDebug() << "getTileFromDb: select error " << QString::fromLocal8Bit((char *)zErrMsg);
                sqlite3_free(zErrMsg);
            }
            else
            {
                rc = sqlite3_prepare_v2(mDb, pSQL, -1, &stmt, NULL);

                if (rc != SQLITE_OK)
                {
                    qgvDebug() << "getTileFromDb: select error " << QString::fromLocal8Bit((char *)zErrMsg);
                    sqlite3_free(zErrMsg);
                }
                else
                {
                    int idx = 0;
                    while (sqlite3_step(stmt) == SQLITE_ROW)
                    {
                        //qgvDebug() << "getTileFromDb: select sqlite3_step: " << QString::fromLocal8Bit((char *)sqlite3_column_text(stmt, db_col_name));
                        tt_name = QString::fromLocal8Bit((char *)sqlite3_column_text(stmt, db_col_name));
                        idx++;
                    }
                }
            }
        }
        else
        {
            qgvDebug() << "getTileFromDb: query is empty!";
        }
    }
    else
    {
        qgvDebug() << "getTileFromDb: db connection not initialized!";
    }
    //qgvDebug() << "getTileFromDb: tt_name: " << tt_name;

    return tt_name;
}

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

#include "QGVLayerTilesOnline.h"
#include "Raster/QGVImage.h"

QGVLayerTilesOnline::QGVLayerTilesOnline()
{
    mCache.init_cache();
}

QGVLayerTilesOnline::~QGVLayerTilesOnline()
{
    qDeleteAll(mRequest);
}

void QGVLayerTilesOnline::request(const QGV::GeoTilePos& tilePos)
{
    Q_ASSERT(QGV::getNetworkManager());
    QString tile_name(tilePosToUrl(tilePos));
    const QUrl url(tile_name);
    QNetworkRequest request(url);


    if (isCache)
    {
        // try to get tile from cache
        QByteArray rawImage = mCache.getTileFromCache(tilePos, tile_name, getName());

        // check if file exists in cache
        if (rawImage.length())
        {
            auto tile = new QGVImage();
            tile->setGeometry(tilePos.toGeoRect());
            tile->loadImage(rawImage);
            tile->setProperty("drawDebug",
                          QString("%1\ntile(%2,%3,%4)")
                                  .arg(tile_name)
                                  .arg(tilePos.zoom())
                                  .arg(tilePos.pos().x())
                                  .arg(tilePos.pos().y()));
            onTile(tilePos, tile);
            return;
        }
        else
        {
            qgvDebug() << "file not in cache...";

            if (isOffline)
            {
                // offline mode
                auto tile_rect = new QGVImage();
                tile_rect->setProperty("drawDebug",
                          QString("%1\ntile(%2,%3,%4)")
                                  .arg(tile_name)
                                  .arg(tilePos.zoom())
                                  .arg(tilePos.pos().x())
                                  .arg(tilePos.pos().y()));

                // offline mode
                //QImage image = mCache.getNoData("NO DATA");
                //image.save("no_data.png", "png", 100);
                tile_rect->setGeometry(tilePos.toGeoRect());
                tile_rect->loadImage(mCache.getNoData("NO DATA"));

                onTile(tilePos, tile_rect);
                return;
            }
        }
    }

    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);

    request.setSslConfiguration(conf);
    request.setRawHeader("User-Agent",
                         "Mozilla/5.0 (Windows; U; MSIE "
                         "6.0; Windows NT 5.1; SV1; .NET "
                         "CLR 2.0.50727)");
    request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

    QNetworkReply* reply = QGV::getNetworkManager()->get(request);

    mRequest[tilePos] = reply;
    //=== connect(object1, SIGNAL(signal(int param)), object2, SLOT(slot()))
    connect(reply, &QNetworkReply::finished, reply, [this, reply, tilePos]() { onReplyFinished(reply, tilePos); });

    qgvDebug() << "request" << url;

}

void QGVLayerTilesOnline::cancel(const QGV::GeoTilePos& tilePos)
{
    removeReply(tilePos);
}

void QGVLayerTilesOnline::onReplyFinished(QNetworkReply* reply, const QGV::GeoTilePos& tilePos)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        // if network error - increment offline counter
        incOfflineCnt();

        if (reply->error() != QNetworkReply::OperationCanceledError)
        {
            qgvCritical() << "ERROR" << reply->errorString();
        }
        removeReply(tilePos);
        return;
    }
    const auto rawImage = reply->readAll();

    // save to file
    if (isCache)
    {
        mCache.putTileToCache(tilePos, reply->url().toString(), getName(), rawImage);
    }

    auto tile = new QGVImage();
    tile->setGeometry(tilePos.toGeoRect());
    tile->loadImage(rawImage);
    tile->setProperty("drawDebug",
                      QString("%1\ntile(%2,%3,%4)")
                              .arg(reply->url().toString())
                              .arg(tilePos.zoom())
                              .arg(tilePos.pos().x())
                              .arg(tilePos.pos().y()));
    removeReply(tilePos);
    onTile(tilePos, tile);
}

void QGVLayerTilesOnline::removeReply(const QGV::GeoTilePos& tilePos)
{
    QNetworkReply* reply = mRequest.value(tilePos, nullptr);
    if (reply == nullptr) {
        return;
    }
    mRequest.remove(tilePos);
    reply->abort();
    reply->close();
    reply->deleteLater();
}
void QGVLayerTilesOnline::setCache(bool mode)
{
    isCache = mode;
    offline_counter = 0;
}

void QGVLayerTilesOnline::setOffline(bool mode)
{
    isOffline = mode;
    offline_counter = 0;
}

void QGVLayerTilesOnline::incOfflineCnt()
{
    offline_counter++;
    if (offline_counter > offline_cnt_max)
    {
        offline_counter = 0;
    }
}

int QGVLayerTilesOnline::loadTilesFromGeo(QGV::GeoRect areaGeoRect, int zoom)
{
    const QPoint topLeft = QGV::GeoTilePos::geoToTilePos(zoom, areaGeoRect.topLeft()).pos();
    const QPoint bottomRight = QGV::GeoTilePos::geoToTilePos(zoom, areaGeoRect.bottomRight()).pos();
    QRect activeRect = QRect(topLeft, bottomRight);

    qgvDebug() << "loadTilesFromGeo: rect" << activeRect.topLeft() << activeRect.bottomRight();

    for (int x = activeRect.left(); x < activeRect.right(); ++x)
    {
        for (int y = activeRect.top(); y < activeRect.bottom(); ++y)
        {
            const auto tilePos = QGV::GeoTilePos(zoom, QPoint(x, y));
            qgvDebug() << "loadTilesFromGeo: tilePos " << tilePos.pos().x() << tilePos.pos().y();
            request(tilePos);
        }
    }


    return 0;
}

/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>
  based on ktp code

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "scamcheckshorturl.h"

#include <libkdepim/misc/broadcaststatus.h>

#include <KGlobal>
#include <KStandardDirs>
#include <KLocalizedString>

#include <qjson/parser.h>

#include <QFile>
#include <QVariantMap>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

using namespace MessageViewer;
QStringList ScamCheckShortUrl::sSupportedServices = QStringList();

ScamCheckShortUrl::ScamCheckShortUrl(QObject *parent)
    : QObject(parent),
      mNetworkAccessManager(new QNetworkAccessManager(this))
{
    loadLongUrlServices();
    connect(mNetworkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slotExpandFinished(QNetworkReply*)));
    connect ( Solid::Networking::notifier(), SIGNAL(statusChanged(Solid::Networking::Status)),
              this, SLOT(slotSystemNetworkStatusChanged(Solid::Networking::Status)) );
    const Solid::Networking::Status networkStatus = Solid::Networking::status();
    slotSystemNetworkStatusChanged(networkStatus);
}

ScamCheckShortUrl::~ScamCheckShortUrl()
{
}

void ScamCheckShortUrl::expandedUrl(const KUrl &url)
{
    if (!mNetworkUp) {
        KPIM::BroadcastStatus::instance()->setStatusMsg( i18n( "No network connection detected, we cannot expand url.") );
        return;
    }
    const QUrl newUrl = QString::fromLatin1("http://api.longurl.org/v2/expand?url=%1&format=json").arg(url.url());

    //qDebug()<<" newUrl "<<newUrl;
    QNetworkReply *reply = mNetworkAccessManager->get(QNetworkRequest(newUrl));
    reply->setProperty("shortUrl", url.url());
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(slotError(QNetworkReply::NetworkError)));
}

void ScamCheckShortUrl::slotExpandFinished(QNetworkReply *reply)
{
    KUrl shortUrl;
    if (!reply->property("shortUrl").isNull()) {
        shortUrl.setUrl(reply->property("shortUrl").toString());
    }
    reply->deleteLater();
    const QString jsonData = QString::fromUtf8(reply->readAll());

    //qDebug() << jsonData;

    QJson::Parser parser;
    bool ok;

    const QMap<QString, QVariant> map = parser.parse(jsonData.toUtf8(), &ok).toMap();
    if (!ok) {
        return;
    }

    KUrl longUrl;
    if (map.contains(QLatin1String("long-url"))) {
        longUrl.setUrl(map.value(QLatin1String("long-url")).toString());
    } else {
        return;
    }

    KPIM::BroadcastStatus::instance()->setStatusMsg( i18n( "Short url \'%1\' redirects to \'%2\'.", shortUrl.url(), longUrl.prettyUrl() ) );
}

void ScamCheckShortUrl::slotError(QNetworkReply::NetworkError error)
{
    Q_EMIT expandUrlError(error);
}

bool ScamCheckShortUrl::isShortUrl(const KUrl &url)
{
    if (!url.path().isEmpty() && QString::compare(url.path(),QLatin1String("/")) && sSupportedServices.contains(url.host())) {
        return true;
    } else  {
        return false;
    }
}

void ScamCheckShortUrl::loadLongUrlServices()
{
    QFile servicesFile(KGlobal::dirs()->findResource("data", QLatin1String("messageviewer/longurlServices.json")));
    if (servicesFile.open(QIODevice::ReadOnly)) {
        const QVariantMap response = QJson::Parser().parse(&servicesFile).toMap();
        sSupportedServices = response.uniqueKeys();
    } else {
        qDebug()<<" json file \'longurlServices.json\' not found";
    }
}

void ScamCheckShortUrl::slotSystemNetworkStatusChanged( Solid::Networking::Status status )
{
    if ( status == Solid::Networking::Connected || status == Solid::Networking::Unknown) {
        mNetworkUp = true;
    } else {
        mNetworkUp = false;
    }
}


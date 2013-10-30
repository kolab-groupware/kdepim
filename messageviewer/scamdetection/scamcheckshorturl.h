/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>
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

#ifndef SCAMCHECKSHORTURL_H
#define SCAMCHECKSHORTURL_H

#include "messageviewer_export.h"

#include <QObject>

#include <KUrl>

#include <QStringList>
#include <QNetworkReply>

class QNetworkAccessManager;
class QNetworkReply;
namespace MessageViewer {
class MESSAGEVIEWER_EXPORT ScamCheckShortUrl : public QObject
{
    Q_OBJECT
public:
    explicit ScamCheckShortUrl(QObject *parent=0);
    ~ScamCheckShortUrl();

    static bool isShortUrl(const KUrl &url);

    void expandedUrl(const KUrl &url);

private Q_SLOTS:
    void slotExpandFinished(QNetworkReply*);
    void slotError(QNetworkReply::NetworkError error);

Q_SIGNALS:
    void urlExpanded(const QString &shortUrl, const QString &expandedUrl);
    void expandUrlError(QNetworkReply::NetworkError error);

private:
    void loadLongUrlServices();

    static QStringList sSupportedServices;
    QNetworkAccessManager *mNetworkAccessManager;
};
}

#endif // SCAMCHECKSHORTURL_H

/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

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

#include "hubicstorageservice.h"
#include "hubicjob.h"

#include <KLocale>
#include <KConfig>
#include <KGlobal>
#include <KConfigGroup>


using namespace PimCommon;

HubicStorageService::HubicStorageService(QObject *parent)
    : PimCommon::StorageServiceAbstract(parent)
{
    readConfig();
}

HubicStorageService::~HubicStorageService()
{
}

void HubicStorageService::readConfig()
{
    KConfigGroup grp(KGlobal::config(), "Hubic Settings");
    mRefreshToken = grp.readEntry("Refresh Token");
}

void HubicStorageService::removeConfig()
{
    KConfigGroup grp(KGlobal::config(), "Hubic Settings");
    grp.deleteGroup();
    KGlobal::config()->sync();
}

void HubicStorageService::authentification()
{
    HubicJob *job = new HubicJob(this);
    connect(job, SIGNAL(authorizationDone(QString)), this, SLOT(slotAuthorizationDone(QString)));
    connect(job, SIGNAL(authorizationFailed(QString)), this, SLOT(slotAuthorizationFailed(QString)));
    job->requestTokenAccess();
}

void HubicStorageService::slotAuthorizationFailed(const QString &errorMessage)
{
    mRefreshToken.clear();
    Q_EMIT authentificationFailed(serviceName(), errorMessage);
}

void HubicStorageService::slotAuthorizationDone(const QString &refreshToken)
{
    mRefreshToken = refreshToken;
    KConfigGroup grp(KGlobal::config(), "Hubic Settings");
    grp.writeEntry("Refresh Token", mRefreshToken);
    grp.sync();
    Q_EMIT authentificationDone(serviceName());
}

void HubicStorageService::listFolder()
{
    if (mRefreshToken.isEmpty()) {
        authentification();
    } else {
        HubicJob *job = new HubicJob(this);
        job->initializeToken(mRefreshToken);
        connect(job, SIGNAL(listFolderDone()), this, SLOT(slotListFolderDone()));
        connect(job, SIGNAL(actionFailed(QString)), SLOT(slotActionFailed(QString)));
        job->listFolder();
    }
}

void HubicStorageService::createFolder(const QString &folder)
{
    if (mRefreshToken.isEmpty()) {
        authentification();
    } else {
        HubicJob *job = new HubicJob(this);
        job->initializeToken(mRefreshToken);
        connect(job, SIGNAL(createFolderDone()), this, SLOT(slotCreateFolderDone()));
        connect(job, SIGNAL(actionFailed(QString)), SLOT(slotActionFailed(QString)));
        job->createFolder(folder);
    }
}

void HubicStorageService::accountInfo()
{
    if (mRefreshToken.isEmpty()) {
        authentification();
    } else {
        HubicJob *job = new HubicJob(this);
        job->initializeToken(mRefreshToken);
        connect(job,SIGNAL(accountInfoDone(PimCommon::AccountInfo)), this, SLOT(slotAccountInfoDone(PimCommon::AccountInfo)));
        connect(job, SIGNAL(actionFailed(QString)), SLOT(slotActionFailed(QString)));
        job->accountInfo();
    }
}

QString HubicStorageService::name()
{
    return i18n("Hubic");
}

void HubicStorageService::uploadFile(const QString &filename)
{
    if (mRefreshToken.isEmpty()) {
        authentification();
    } else {
        HubicJob *job = new HubicJob(this);
        job->initializeToken(mRefreshToken);
        connect(job, SIGNAL(uploadFileDone()), this, SLOT(slotUploadFileDone()));
        connect(job, SIGNAL(actionFailed(QString)), SLOT(slotActionFailed(QString)));
        connect(job, SIGNAL(uploadFileProgress(qint64,qint64)), SLOT(slotUploadFileProgress(qint64,qint64)));
        job->uploadFile(filename);
    }
}

QString HubicStorageService::description()
{
    return i18n("Hubic is a file hosting service operated by Ovh, Inc. that offers cloud storage, file synchronization, and client software.");
}

QUrl HubicStorageService::serviceUrl()
{
    return QUrl(QLatin1String("https://hubic.com"));
}

QString HubicStorageService::serviceName()
{
    return QLatin1String("hubic");
}

QString HubicStorageService::iconName()
{
    return QString();
}

void HubicStorageService::shareLink(const QString &root, const QString &path)
{
    if (mRefreshToken.isEmpty()) {
        authentification();
    } else {
        HubicJob *job = new HubicJob(this);
        job->initializeToken(mRefreshToken);
        connect(job, SIGNAL(shareLinkDone(QString)), this, SLOT(slotShareLinkDone(QString)));
        connect(job, SIGNAL(actionFailed(QString)), SLOT(slotActionFailed(QString)));
        job->shareLink(root, path);
    }
}

QString HubicStorageService::storageServiceName() const
{
    return serviceName();
}

KIcon HubicStorageService::icon() const
{
    return KIcon();
}

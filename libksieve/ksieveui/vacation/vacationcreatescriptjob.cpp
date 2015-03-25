/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

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

#include "vacationcreatescriptjob.h"
#include "vacationutils.h"
#include <kmanagesieve/sievejob.h>

#include <KMessageBox>
#include <KLocalizedString>
#include <KDebug>

using namespace KSieveUi;

VacationCreateScriptJob::VacationCreateScriptJob(QObject *parent)
    : QObject(parent),
      mActivate(false),
      mWasActive(false),
      mSieveJob(0)
    , mKep14Support(false)
{

}

VacationCreateScriptJob::~VacationCreateScriptJob()
{

}

void VacationCreateScriptJob::setStatus(bool activate, bool wasActive)
{
    mActivate = activate;
    mWasActive = wasActive;
}

void VacationCreateScriptJob::setServerName(const QString &servername)
{
    mServerName = servername;
}

const QString &VacationCreateScriptJob::serverName() const
{
    return mServerName;
}

void VacationCreateScriptJob::setKep14Support(bool kep14Support)
{
    mKep14Support = kep14Support;
}

void VacationCreateScriptJob::setServerUrl(const KUrl &url)
{
    mUrl = url;
}

void VacationCreateScriptJob::setScript(const QString &script)
{
    mScript = script;
}

void VacationCreateScriptJob::start()
{
    if (mUrl.isEmpty()) {
        qDebug()<<" server url is empty";
        deleteLater();
        return;
    }
    mSieveJob = KManageSieve::SieveJob::get(mUrl);
    mSieveJob->setInteractive(false);
    connect(mSieveJob, SIGNAL(gotScript(KManageSieve::SieveJob*,bool,QString,bool)),
            SLOT(slotGetScript(KManageSieve::SieveJob*,bool,QString,bool)));
}

void VacationCreateScriptJob::slotGetScript(KManageSieve::SieveJob *job, bool success, const QString &oldScript, bool active)
{
    QString script = mScript;
    if (success || !oldScript.trimmed().isEmpty()) {
        script = VacationUtils::mergeRequireLine(oldScript, mScript);
        script = VacationUtils::updateVacationBlock(oldScript,mScript);
    }
    if (mKep14Support) {
        mSieveJob = KManageSieve::SieveJob::put( mUrl, mScript, false, false );
    } else {
        mSieveJob = KManageSieve::SieveJob::put( mUrl, mScript, mActivate, false );         //Never deactivate
    }
    if ( mActivate )
        connect( mSieveJob, SIGNAL(gotScript(KManageSieve::SieveJob*,bool,QString,bool)),
                 SLOT(slotPutActiveResult(KManageSieve::SieveJob*,bool)) );
    else
        connect( mSieveJob, SIGNAL(gotScript(KManageSieve::SieveJob*,bool,QString,bool)),
                 SLOT(slotPutInactiveResult(KManageSieve::SieveJob*,bool)) );
}


void VacationCreateScriptJob::slotPutActiveResult( KManageSieve::SieveJob * job, bool success )
{
    handlePutResult( job, success, true );
}

void VacationCreateScriptJob::slotPutInactiveResult( KManageSieve::SieveJob * job, bool success )
{
    handlePutResult( job, success, false );
}

void VacationCreateScriptJob::handlePutResult( KManageSieve::SieveJob *, bool success, bool activated )
{
    if ( success )
        KMessageBox::information( 0, activated
                                  ? i18n("Sieve script installed successfully on the server \'%1\'.\n"
                                         "Out of Office reply is now active.", mServerName)
                                  : i18n("Sieve script installed successfully on the server \'%1\'.\n"
                                         "Out of Office reply has been deactivated.", mServerName) );

    kDebug() << "( ???," << success << ", ? )";
    mSieveJob = 0; // job deletes itself after returning from this slot!
    Q_EMIT result( success );
    Q_EMIT scriptActive( activated, mServerName );
    deleteLater();
}

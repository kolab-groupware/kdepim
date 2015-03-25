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

#ifndef VACATIONCREATESCRIPTJOB_H
#define VACATIONCREATESCRIPTJOB_H

#include <QObject>

#include "ksieveui_export.h"

#include <KUrl>

namespace KManageSieve {
class SieveJob;
}

namespace KSieveUi {
class KSIEVEUI_EXPORT VacationCreateScriptJob : public QObject
{
    Q_OBJECT
public:
    explicit VacationCreateScriptJob(QObject *parent=0);
    ~VacationCreateScriptJob();

    void start();

    void setServerUrl(const KUrl &url);
    void setScript(const QString &script);
    void setServerName(const QString &servername);
    const QString &serverName() const;
    void setStatus(bool activate, bool wasActive);
    void setKep14Support(bool kep14Support);
    QString updateVacationBlock(QString oldScript, QString mScript);
    QString mergeRequireLine(QString oldScript, QString mScript);

Q_SIGNALS:
    void result(bool);
    void scriptActive(bool activated, const QString &serverName);

private slots:
    void slotPutActiveResult(KManageSieve::SieveJob *job, bool success);
    void slotPutInactiveResult(KManageSieve::SieveJob *job, bool success);
    void slotGetScript(KManageSieve::SieveJob *job, bool success, const QString &oldScript, bool active);

private:
    void handlePutResult(KManageSieve::SieveJob *, bool success, bool activated);
    KUrl mUrl;
    QString mScript;
    QString mServerName;
    bool mActivate;
    bool mWasActive;
    bool mKep14Support;
    KManageSieve::SieveJob *mSieveJob;
};
}

#endif // VACATIONCREATESCRIPTJOB_H

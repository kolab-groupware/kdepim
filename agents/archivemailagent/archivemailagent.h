/*
  Copyright (c) 2012-2013 Montel Laurent <montel@kde.org>

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

#ifndef ARCHIVEMAILAGENT_H
#define ARCHIVEMAILAGENT_H

#include <akonadi/agentbase.h>

class QTimer;


class ArchiveMailManager;
class ArchiveMailInfo;

class ArchiveMailAgent : public Akonadi::AgentBase, public Akonadi::AgentBase::ObserverV3
{
    Q_OBJECT

public:
    explicit ArchiveMailAgent( const QString &id );
    ~ArchiveMailAgent();

    void showConfigureDialog(qlonglong windowId = 0);
    void printArchiveListInfo();

    void setEnableAgent(bool b);
    bool enabledAgent() const;


Q_SIGNALS:
    void archiveNow(ArchiveMailInfo *info);
    void needUpdateConfigDialogBox();

public Q_SLOTS:
    void configure( WId windowId );
    void reload();
    void pause();
    void resume();

private Q_SLOTS:
    void mailCollectionRemoved( const Akonadi::Collection &collection );

protected:
    void doSetOnline(bool online);

private:
    QTimer *mTimer;
    ArchiveMailManager *mArchiveManager;
};


#endif /* ARCHIVEMAILAGENT_H */


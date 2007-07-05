/* RecordConduit.cc			KPilot
**
** Copyright (C) 2007 by Bertjan Broeksema
** Copyright (C) 2007 by Jason "vanRijn" Kasper
*/

/*
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 2.1 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program in a file called COPYING; if not, write to
** the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
** MA 02110-1301, USA.
*/

/*
** Bug reports and questions can be sent to kde-pim@kde.org
*/
#include "options.h"

#include "recordconduit.h"
#include "idmapping.h"
#include "pcdataproxy.h"
#include "hhdataproxy.h"
#include "record.h"

RecordConduit::RecordConduit( KPilotLink *o, const QStringList &a
	, const QString &databaseName, const QString &conduitName ) :
	ConduitAction(o, a),
	fHHDataProxy(0L),
	fBackupDataProxy(0L),
	fPCDataProxy(0L)
{
	fDatabaseName = databaseName;
	fConduitName = conduitName;
}

RecordConduit::~RecordConduit()
{
	delete fHHDataProxy;
	delete fBackupDataProxy;
	delete fPCDataProxy;
}

/* virtual */ bool RecordConduit::exec()
{
	FUNCTIONSETUP;
	
	loadSettings();
	
	// Try to open the handheld database and the local backup database.
	bool retrieved = false;
	// FIXME: It was this: CSL1( "fDatabaseName" .... I don't how to handle
	// QString properly at this point.
	bool hhDatabaseOpen = openDatabases( fDatabaseName, &retrieved );
	// If retrieved is true there is no local backup database so a first sync 
		// should be done then (see 6.3.2).
	bool backupDatabaseOpen = !retrieved;
	setFirstSync( retrieved );
	
	initDataProxies();
	
	// For the pc data proxy we can only after initilisation know if it could be
	// opened.
	bool pcDatabaseOpen = fPCDataProxy->isOpen();
	
	// See 6.2
	// FIXME: Retrieve the username.
	fMapping = new IDMapping( QString(""), fConduitName );
	if( !fMapping->isValid( fBackupDataProxy->ids() ) )
	{
		setFirstSync( true );
	}
	
	// Determine syncmode, see 6.3
	// FIXME: Is this the most efficient way, or is it even possible what i do 
	//        here?
	if( hhDatabaseOpen && pcDatabaseOpen && backupDatabaseOpen )
	{
		changeSync( SyncMode::eHotSync ); // 6.3.1
		fHHDataProxy->setIterateMode( DataProxy::Modified );
		fPCDataProxy->setIterateMode( DataProxy::Modified );
		hotSync();
	}
	else if( hhDatabaseOpen && pcDatabaseOpen && !backupDatabaseOpen )
	{
		setFirstSync( true ); // 6.3.2
		fHHDataProxy->setIterateMode( DataProxy::All );
		fPCDataProxy->setIterateMode( DataProxy::All );
		//firstSync();
	}
	else if( hhDatabaseOpen && !pcDatabaseOpen )
	{
		changeSync( SyncMode::eCopyHHToPC ); // 6.3.3 and 6.3.4
		fHHDataProxy->setIterateMode( DataProxy::All );
		fPCDataProxy->setIterateMode( DataProxy::All );
		//copyHHToPC();
	}
	else if( !hhDatabaseOpen && pcDatabaseOpen )
	{
		changeSync( SyncMode::eCopyPCToHH ); // 6.3.5 and 6.3.6
		fHHDataProxy->setIterateMode( DataProxy::All );
		fPCDataProxy->setIterateMode( DataProxy::All );
		//copyPCToHH();
	}
	else
	{
		emit logError( i18n( "Failed to open the pc database and the handheld "
			"datbase, no data to sync." ) );
		return false; // 6.3.7 and 6.3.8
	}
	
	// Sync finished, clean up things.
	fHHDataProxy->syncFinished();
	fPCDataProxy->syncFinished();
	
	fMapping->setLastSyncedDate( QDateTime::currentDateTime() );
	if( !fMapping->isValid( fHHDataProxy->ids() ) )
	{
		// TODO: Warn the user.
		return false;
	}
	
	if( !checkVolatility() )
	{
		// volatility bounds are exceeded or the user did not want to proceed.
		return false;
	}
	
	/*
	 * If from this point something goes wrong (which shouldn't because we did our
	 * very best to deliver sane data) some of the data (mapping, hh database or
	 * pc database) will be corrupted.
	 */
	if( !fMapping->commit() )
	{
		// Solution: restore mapping file backup -> TODO: make a backup of the file.
		fMapping->rollback();
		return false;
	}
	
	// If commit fails every commit should be undone and the user should be 
	// notified about the failure.
	if( createBackupDatabase() )
	{
		if( fHHDataProxy->commit() )
		{
			if( !fPCDataProxy->commit() )
			{
				fPCDataProxy->rollback();
				fHHDataProxy->rollback();
				fMapping->rollback();
				// TODO: notify user.
				return false;
			}
			else
			{
				return true;
			}
		}
		else
		{
			fHHDataProxy->rollback();
			fMapping->rollback();
			// TODO: notify user.
			return false;
		}
	} 
	else
	{
		fMapping->rollback();
		// TODO: notify user.
		return false;
	}
	
	return true;
}

bool RecordConduit::checkVolatility()
{
	FUNCTIONSETUP;
	#warning Not implemented!
	return false;
	
	/*
	const CUDCounter *fCtrHH = fHHDataProxy->counter();
	const CUDCounter *fCtrPC = fPCDataProxy->counter();
	if (fCtrHH && fCtrPC)
	{
		addSyncLogEntry(fCtrHH->moo() +'\n',false);
		DEBUGKPILOT << fname << ": " << fCtrHH->moo() << endl;
		addSyncLogEntry(fCtrPC->moo() +'\n',false);
		DEBUGKPILOT << fname << ": " << fCtrPC->moo() << endl;

		// STEP2 of making sure we don't delete our little user's
		// precious data...
		// sanity checks for handheld...
		int hhVolatility = fCtrHH->percentDeleted() +
				 fCtrHH->percentUpdated() + fCtrHH->percentCreated();

		int pcVolatility = fCtrPC->percentDeleted() +
				 fCtrPC->percentUpdated() + fCtrPC->percentCreated();

		// TODO: allow user to configure this...
		// this is a percentage...
		int allowedVolatility = 70;

		QString caption = i18n( "Large Changes Detected" );
		// args are already i18n'd
		KLocalizedString template_query = ki18n("The %1 conduit has made a "
			         "large number of changes to your %2.  Do you want "
			         "to allow this change?\nDetails:\n\t%3");

		if (hhVolatility > allowedVolatility)
		{
			QString query = template_query.subs(fConduitName)
				.subs(fCtrHH->type()).subs(fCtrHH->moo()).toString();

			DEBUGKPILOT << fname << ": high volatility."
				<< "  Check with user: [" << query
				<< "]." << endl;
		*/
			/*
			int rc = questionYesNo(query, caption,
				QString::null, 0 );
			if (rc == KMessageBox::Yes)
			{
				// TODO: add commit and rollback code.
				// note: this will require some thinking,
				// since we have to undo changes to the
				// pilot databases, changes to the PC
				// resources, changes to the mappings files
				// (record id mapping, etc.)
			}
			*/
		//}
		/*
		if (pcVolatility > allowedVolatility)
		{
			QString query = template_query.subs(fConduitName)
				.subs(fCtrPC->type()).subs(fCtrPC->moo()).toString();

			DEBUGKPILOT << fname << ": high volatility."
				<< "  Check with user: [" << query
				<< "]." << endl;
		}
	}
	*/
}

void RecordConduit::hotSync()
{
	FUNCTIONSETUP;
	// Walk through all modified hand held records. The proxy is responsible for
	// serving the right records.
	while( fHHDataProxy->hasNext() )
	{
		Record *hhRecord = fHHDataProxy->next();
		Record *backupRecord = fBackupDataProxy->readRecordById( hhRecord->id() );
		Record *pcRecord = 0L;
		
		QVariant pcRecordId = fMapping->recordId( hhRecord->id() );
		if( pcRecordId.isValid() ) {
			// There is a mapping.
			pcRecord = fPCDataProxy->readRecordById( pcRecordId );
		}
		
		syncRecords( hhRecord, backupRecord, pcRecord );
	}
	
	// Walk through all modified pc records. The proxy is responsible for
	// serving the right records.
	while( fPCDataProxy->hasNext() )
	{
		Record *pcRecord = fPCDataProxy->next();
		Record *backupRecord = 0L;
		Record *hhRecord = 0L;
		
		QVariant hhRecordId = fMapping->recordId( pcRecord->id() );
		if( hhRecordId.isValid() ) {
			// There is a mapping.
			backupRecord = fBackupDataProxy->readRecordById( pcRecord->id() );
			hhRecord = fHHDataProxy->readRecordById( pcRecord->id() );
		}
		
		syncRecords( hhRecord, backupRecord, pcRecord );
	}
}

void RecordConduit::syncRecords( Record *pcRecord, Record *backupRecord,
			Record *hhRecord )
{
	FUNCTIONSETUP;
	#warning implemented, but needs work
	
	// Two records for which we seem to have a mapping.
	if( hhRecord && pcRecord )
	{
		// Cases: 6.5.1 (fullSync), 6.5.3
		if( hhRecord->isModified() || isFullSync() )
		{
			// Case: 6.5.9
			if( pcRecord->isModified() )
			{
				solveConflict( hhRecord, pcRecord );
			}
			else
			{
				//            from      to
				syncFields( hhRecord, pcRecord );
			}
		}
		// Case: 6.5.6
		else if( pcRecord->isModified() )
		{
			//            from      to
			syncFields( pcRecord, hhRecord );
		}
		/*
		else
		{
			// Case: 6.5.1 (hotSync)
		}
		*/
	}
	
	if( hhRecord )
	{
		if( backupRecord )
		{
			// Case: 6.5.11
			if( hhRecord->isModified() )
			{
				// Pc record deleted, hh record modified.
				solveConflict( 0L, hhRecord );
			}
			// Case: 6.5.7
			else
			{
				fHHDataProxy->remove( hhRecord->id() );
				fMapping->remove( hhRecord->id() );
			}
		}
		// Case: 6.5.2 (and 6.5.8 if the conduit iterates over the HH data proxy
		// first )
		else
		{
			// Warning id is a temporary id. Only after commit we know what id is
			// assigned to the record. So on commit the proxy should get the mapping
			// so that it can change the mapping.
			QVariant id = fPCDataProxy->create( hhRecord );
			fMapping->map( hhRecord->id(), id );
		}
	}
	
	if( pcRecord )
	{
		if( backupRecord )
		{
			// Case: 6.5.10
			if( pcRecord->isModified() )
			{
				solveConflict( pcRecord, 0L );
			}
			// Case: 6.5.4
			else
			{
				fPCDataProxy->remove( pcRecord->id() );
				fMapping->remove( pcRecord->id() );
			}
		}
		// Case: 6.5.5 (and 6.5.8 if the conduit iterates over the PC data proxy 
		// first )
		else
		{
			QVariant id = fHHDataProxy->create( pcRecord );
			fMapping->map( id, pcRecord->id() );
		}
	}
	
	// For completeness: Case 6.5.12
	/*
	if( backupRecord )
	{
		return true;
	}
	
	return false;
	*/
}

bool RecordConduit::syncFields( Record *to, Record *from )
{
	FUNCTIONSETUP;
	
	// This shouldn't happen.
	if( !to || !from )
	{
		DEBUGKPILOT << fname << ": error, one of the two records is zero!" << endl;
		return false;
	}
	
	QStringList fields = from->fields();
	QStringListIterator it( fields );
	while( it.hasNext() )
	{
		QString field = it.next();
		bool result = to->setValue( field, from->value( field ) );
		if( !result )
		{
			DEBUGKPILOT << fname << ": error, field " << field << " does not exists ";
			DEBUGKPILOT	<< "or the value given is not valid!" << endl;
			return false;
		}
	}
	
	return true;
}

void RecordConduit::solveConflict( Record *pcRecord, Record *hhRecord )
{
	FUNCTIONSETUP;
	Q_UNUSED(pcRecord);
	Q_UNUSED(hhRecord);
	#warning Not implemented!
}

bool RecordConduit::askConfirmation(const QString & volatilityMessage)
{
	FUNCTIONSETUP;
	#warning Not implemented!
	Q_UNUSED(volatilityMessage);
	return false;
}

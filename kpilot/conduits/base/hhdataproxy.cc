/* hhdataproxy.cc			KPilot
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

#include "hhdataproxy.h"
#include "hhrecord.h"

#include "pilotDatabase.h"
#include "pilotRecord.h"
#include "options.h"

HHDataProxy::HHDataProxy( PilotDatabase *db ) : fDatabase( db )
{
}

void HHDataProxy::resetSyncFlags()
{
	FUNCTIONSETUP;
	
	fDatabase->resetSyncFlags();
}

void HHDataProxy::commitCreate( Record *rec )
{
	FUNCTIONSETUP;
	
	if( rec )
	{
		if( HHRecord *hhRec = static_cast<HHRecord*>( rec ) )
		{
			// Set the id to 0 to make sure that the database asigns a valid id to the
			// record.
			hhRec->setId( 0 );
			fDatabase->writeRecord( hhRec->pilotRecord() );
		}
		else
		{
			DEBUGKPILOT << "Record " << rec->id() << " is not of type HHRecord*.";
		}
	}
}

void HHDataProxy::commitUpdate( Record *rec )
{
	FUNCTIONSETUP;

	if( rec )
	{
		if( HHRecord *hhRec = static_cast<HHRecord*>( rec ) )
		{
			fDatabase->writeRecord( hhRec->pilotRecord() );
		}
		else
		{
			DEBUGKPILOT << "Record " << rec->id() << " is not of type HHRecord*.";
		}
	}
}

void HHDataProxy::commitDelete( Record *rec )
{
	FUNCTIONSETUP;
	
	if( !rec )
	{
		return;
	}
	else
	{
		if( HHRecord *hhRec = static_cast<HHRecord*>( rec ) )
		{
			fDatabase->deleteRecord( hhRec->pilotRecord()->id() );
		}
		else
		{
			DEBUGKPILOT << "Record " << rec->id() << " is not of type HHRecord*.";
		}
	}
}

bool HHDataProxy::isOpen() const
{
	FUNCTIONSETUP;
	
	return fDatabase->isOpen();
}

void HHDataProxy::loadAllRecords()
{
	FUNCTIONSETUP;
	
	int index = 0;
	
	PilotRecord *pRec = fDatabase->readRecordByIndex( index );
	
	while( pRec )
	{
		// Create a record object.
		Record *rec = createHHRecord( pRec );
		fRecords.insert( rec->id(), rec );
		
		// Read the next one.
		pRec = fDatabase->readRecordByIndex( ++index );
	}
}

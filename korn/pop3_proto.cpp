/*
 * Copyright (C) 2005, Mart Kelder (mart.kde@hccnet.nl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#include "pop3_proto.h"

#include "account_input.h"

#include <qwidget.h>
#include <qobject.h>
#include <qstringlist.h>
#include <q3ptrvector.h>
#include <q3ptrlist.h>

void Pop3_Protocol::configFillGroupBoxes( QStringList* groupBoxes ) const
{
	groupBoxes->append( "Server" );
	groupBoxes->append( "Identify" );
}

void Pop3_Protocol::configFields( Q3PtrVector< QWidget >* vector, const QObject* configDialog, Q3PtrList< AccountInput > * result ) const
{
	QMap< QString, QString > encrList;
	encrList.insert( "ssl", i18n( "Secure Socket Layer" ) );
	encrList.insert( "tls=auto", i18n( "TLS if possible" ) );
	encrList.insert( "tls=on", i18n( "Always TLS" ) );
	encrList.insert( "tls=off", i18n( "Never TLS" ) );
	
	QMap< QString, QString > authList;
	authList.insert( "", i18n( "Plain" ) );
	authList.insert( "auth=APOP", i18n( "APOP" ) );

	result->append( new TextInput( (QWidget*)vector->at( 0 ), i18n( "Server" ), TextInput::text, "", "server" ) );
	result->append( new TextInput( (QWidget*)vector->at( 0 ), i18n( "Port" ), 0, 65535, "110", "port" ) );
	result->append( new ComboInput( (QWidget*)vector->at( 0 ), i18n( "Encryption" ), encrList, "tls=auto", "encryption" ) );
	QObject::connect( (QObject*)result->last()->rightWidget(), SIGNAL( activated( int) ),
	                  configDialog, SLOT( slotSSLChanged() ) );
	
	result->append( new TextInput( (QWidget*)vector->at( 1 ), i18n( "Username" ), TextInput::text, "", "username" ) );
	result->append( new TextInput( (QWidget*)vector->at( 1 ), i18n( "Password" ), TextInput::password, "", "password" ) );
	result->append( new CheckboxInput( (QWidget*)vector->at( 1 ), i18n( "Save password" ), "true", "savepassword" ) );
	QObject::connect( (QObject*)result->last()->rightWidget(), SIGNAL( toggled( bool ) ),
			  (QObject*)result->prev()->rightWidget(), SLOT( setEnabled( bool ) ) );
	result->last()->setValue( "false" );
	result->append( new ComboInput( (QWidget*)vector->at( 1 ), i18n( "Authentication" ), authList, "", "auth" ) );
}

void Pop3_Protocol::readEntries( QMap< QString, QString >* map, QMap< QString, QString > *metadata ) const
{
	if( map->contains( "ssl" ) && *map->find( "ssl" ) == "true" )
		map->insert( "encryption", "ssl" );
	if( metadata->contains( "tls" ) )
		map->insert( "encryption", QString( "tls=%1" ).arg( *metadata->find( "tls" ) ) );
	if( metadata->contains( "auth" ) )
		map->insert( "auth", QString( "auth=APOP" ) );
}

void Pop3_Protocol::writeEntries( QMap< QString, QString >* map ) const
{
	QString metadata;
	if( map->contains( "encryption" ) )
	{
		if( *map->find( "encryption" ) == "ssl" )
			map->insert( "ssl", "true" );
		else
		{
			map->insert( "ssl", "false" );
			metadata += *map->find( "encryption" );
		}
		map->erase( "encryption" );
	}

	if( map->contains( "auth" ) )
	{
		if( !metadata.isEmpty() && ! (*map->find( "auth" )).isEmpty() )
			metadata += ",";
		metadata += *map->find( "auth" );
		map->erase( "auth" );
	}

	map->insert( "metadata", metadata );

	clearFields( map, KIO_Protocol::mailbox );
}


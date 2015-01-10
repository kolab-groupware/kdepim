/* -*- mode: C++; c-file-style: "gnu" -*-
  Copyright (c) 2010 Volker Krause <vkrause@kde.org>

  Based on kmail/kmlineeditspell.h/cpp
  Copyright (c) 1997 Markus Wuebben <markus.wuebben@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/


#include "composerlineedit.h"

#include "addressline/recentaddresses.h"
#include "settings/messagecomposersettings.h"
#include "messageviewer/utils/autoqpointer.h"

#include <messagecore/utils/stringutil.h>

#include <kpimutils/email.h>
#include <kabc/vcarddrag.h>
#include <kabc/contactgroup.h>
#include <kabc/vcardconverter.h>

#include <kio/netaccess.h>
#include <kmenu.h>
#include <kurl.h>
#include <kmessagebox.h>
#include <kcompletionbox.h>
#include <klocale.h>

#include <QFile>
#include <QCursor>
#include <QKeyEvent>
#include <QDropEvent>
#include <kabc/contactgrouptool.h>
#include <Akonadi/Contact/ContactGroupExpandJob>
#include <Akonadi/Contact/ContactGroupSearchJob>
#include <QtCore/QBuffer>

using namespace MessageComposer;

ComposerLineEdit::ComposerLineEdit(bool useCompletion, QWidget *parent)
    : KPIM::AddresseeLineEdit(parent, useCompletion),
      m_recentAddressConfig( MessageComposerSettings::self()->config() ),
      mAutoGroupExpand(false),
      mExpandIntern(true)
{
    allowSemicolonAsSeparator( MessageComposerSettings::allowSemicolonAsAddressSeparator() );
    loadContacts();
    connect( this, SIGNAL(editingFinished()), SLOT(slotEditingFinished()) );
    connect( this, SIGNAL(textCompleted()), SLOT(slotEditingFinished()) );

    KConfigGroup group( KGlobal::config(), "AddressLineEdit" );
    mAutoGroupExpand = group.readEntry( "AutoGroupExpand", false );
}


//-----------------------------------------------------------------------------
void ComposerLineEdit::keyPressEvent(QKeyEvent *e)
{
    if ((e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) &&
            !completionBox()->isVisible())
    {
        emit focusDown();
        AddresseeLineEdit::keyPressEvent(e);
        return;
    } else if (e->key() == Qt::Key_Up) {
        emit focusUp();
        return;
    } else if (e->key() == Qt::Key_Down) {
        emit focusDown();
        return;
    }
    AddresseeLineEdit::keyPressEvent(e);
}


void ComposerLineEdit::insertEmails( const QStringList & emails )
{
    if ( emails.empty() )
        return;

    QString contents = text();
    if ( !contents.isEmpty() )
        contents += QLatin1Char(',');
    // only one address, don't need kpopup to choose
    if ( emails.size() == 1 ) {
        setText( contents + emails.front() );
        return;
    }
    //multiple emails, let the user choose one
    KMenu menu( this );
    menu.setObjectName( QLatin1String("Addresschooser") );
    for ( QStringList::const_iterator it = emails.constBegin(), end = emails.constEnd() ; it != end; ++it )
        menu.addAction( *it );
    const QAction *result = menu.exec( QCursor::pos() );
    if ( !result )
        return;
    setText( contents + KGlobal::locale()->removeAcceleratorMarker( result->text() ) );
}

#ifndef QT_NO_DRAGANDDROP
void ComposerLineEdit::dropEvent(QDropEvent *event)
{
    const QMimeData *md = event->mimeData();

    // Case one: The user dropped a text/directory (i.e. vcard), so decode its
    //           contents
    if ( KABC::VCardDrag::canDecode( md ) ) {
        KABC::Addressee::List list;
        KABC::VCardDrag::fromMimeData( md, list );

        KABC::Addressee::List::ConstIterator ait;
        KABC::Addressee::List::ConstIterator end( list.constEnd() );
        for ( ait = list.constBegin(); ait != end; ++ait ){
            insertEmails( (*ait).emails() );
        }
    }

    // Case two: The user dropped a list or Urls.
    // Iterate over that list. For mailto: Urls, just add the addressee to the list,
    // and for other Urls, download the Url and assume it points to a vCard
    else if ( KUrl::List::canDecode( md ) ) {
        KUrl::List urls = KUrl::List::fromMimeData( md );
        KABC::Addressee::List list;

        foreach ( const KUrl& url, urls ) {

            // First, let's deal with mailto Urls. The path() part contains the
            // email-address.
            if ( url.protocol() == QLatin1String("mailto") ) {
                KABC::Addressee addressee;
                addressee.insertEmail( KPIMUtils::decodeMailtoUrl( url ), true /* preferred */ );
                list += addressee;
            }

            // Otherwise, download the vCard to which the Url points
            else {
                KABC::VCardConverter converter;
                QString fileName;
                if ( KIO::NetAccess::download( url, fileName, parentWidget() ) ) {
                    QFile file( fileName );
                    if (file.open( QIODevice::ReadOnly ) ) {
                        QByteArray data = file.readAll();
                        file.close();
                        list += converter.parseVCards( data );
                        KIO::NetAccess::removeTempFile( fileName );

                        if( list.isEmpty() ) { // try to parse a contact group
                            KABC::ContactGroup group;
                            QBuffer dataStream( &data );
                            dataStream.open( QIODevice::ReadOnly );
                            QString error;
                            if( KABC::ContactGroupTool::convertFromXml( &dataStream, group, &error ) ) {
                                Akonadi::ContactGroupExpandJob* expandJob = new Akonadi::ContactGroupExpandJob( group );
                                connect( expandJob, SIGNAL(result(KJob*)), this, SLOT(groupExpandResult(KJob*)) );
                                expandJob->start();
                            }
                        }
                    }
                } else {
                    const QString caption( i18n( "vCard Import Failed" ) );
                    const QString text = i18n( "<qt>Unable to access <b>%1</b>.</qt>", url.url() );
                    KMessageBox::error( parentWidget(), text, caption );
                }
            }
        }

        // Now, let the user choose which addressee to add.
        foreach( const KABC::Addressee& addressee, list ) {
            insertEmails( addressee.emails() );
        }
    }

    // Case three: Let AddresseeLineEdit deal with the rest
    else {
        KPIM::AddresseeLineEdit::dropEvent( event );
    }
}
#endif

void ComposerLineEdit::groupExpandResult( KJob* job )
{
    Akonadi::ContactGroupExpandJob *expandJob = qobject_cast<Akonadi::ContactGroupExpandJob*>( job );

    if( !expandJob )
        return;

    const KABC::Addressee::List contacts = expandJob->contacts();
    foreach( const KABC::Addressee& addressee, contacts ) {
        if (mExpandIntern || text().isEmpty()) {
            insertEmails( QStringList() << addressee.fullEmail() );
        } else {
            emit addAddress(addressee.fullEmail());
        }
    }

    job->deleteLater();
}

void ComposerLineEdit::slotEditingFinished()
{
    foreach(KJob *job, mMightBeGroupJobs) {
        disconnect(job);
        job->deleteLater();
    }

    mMightBeGroupJobs.clear();
    mGroups.clear();

    if (!text().isEmpty()) {
        QStringList addresses = KPIMUtils::splitAddressList(text());
        foreach(QString address, addresses) {
            Akonadi::ContactGroupSearchJob *job = new Akonadi::ContactGroupSearchJob();
            job->setQuery( Akonadi::ContactGroupSearchJob::Name, address);
            connect( job, SIGNAL(result(KJob*)), this, SLOT(slotGroupSearchResult(KJob*)) );
            mMightBeGroupJobs.append(job);
        }
    }
}

void ComposerLineEdit::slotGroupSearchResult(KJob *job)
{
  Akonadi::ContactGroupSearchJob *searchJob = qobject_cast<Akonadi::ContactGroupSearchJob*>( job );

  Q_ASSERT(mMightBeGroupJobs.contains(searchJob));
  mMightBeGroupJobs.removeOne(searchJob);

  const KABC::ContactGroup::List contactGroups = searchJob->contactGroups();
  if ( contactGroups.isEmpty() ) {
    return; // Nothing todo, probably a normal email address was entered
  }

  mGroups.append(contactGroups);
  searchJob->deleteLater();

  if (mAutoGroupExpand) {
      expandGroups();
  }
}

void ComposerLineEdit::expandGroups()
{
    QStringList addresses = KPIMUtils::splitAddressList(text());

    foreach(KABC::ContactGroup group, mGroups) {
        Akonadi::ContactGroupExpandJob* expandJob = new Akonadi::ContactGroupExpandJob( group );
        connect( expandJob, SIGNAL(result(KJob*)), this, SLOT(groupExpandResult(KJob*)) );
        addresses.removeAll(group.name());
        expandJob->start();
    }
    setText(addresses.join(QLatin1String(", ")));
    mGroups.clear();
}

void ComposerLineEdit::slotToggleExpandGroups()
{
    mAutoGroupExpand = !mAutoGroupExpand;
    KConfigGroup group( KGlobal::config(), "AddressLineEdit" );
    group.writeEntry( "AutoGroupExpand", mAutoGroupExpand );
}


#ifndef QT_NO_CONTEXTMENU
void ComposerLineEdit::contextMenuEvent( QContextMenuEvent*e )
{
    QPointer<QMenu> popup = createStandardContextMenu();
    if ( popup ) { // can be 0 on platforms with only a touch interface
        popup->addSeparator();
        QAction* act = popup->addAction( i18n( "Edit Recent Addresses..." ));
        connect(act,SIGNAL(triggered(bool)), SLOT(editRecentAddresses()) );

        popup->addSeparator();
        act = popup->addAction( i18n( "Automatically expand groups" ));
        act->setCheckable(true);
        act->setChecked(mAutoGroupExpand);
        connect(act,SIGNAL(triggered(bool)), SLOT(slotToggleExpandGroups()) );

        if (mGroups.count() > 0) {
            act = popup->addAction( i18n( "Expand Groups..." ));
            connect(act,SIGNAL(triggered(bool)), SLOT(expandGroups()) );
        }
        popup->exec( e->globalPos() );
        delete popup;
    }
}
#endif

void ComposerLineEdit::editRecentAddresses()
{
    MessageViewer::AutoQPointer<KPIM::RecentAddressDialog> dlg( new KPIM::RecentAddressDialog( this ) );
    dlg->setAddresses( KPIM::RecentAddresses::self( m_recentAddressConfig )->addresses() );
    if ( dlg->exec() && dlg ) {
        KPIM::RecentAddresses::self( m_recentAddressConfig )->clear();
        dlg->addAddresses(m_recentAddressConfig);
        loadContacts();
    }
}


//-----------------------------------------------------------------------------
void ComposerLineEdit::loadContacts()
{
    //AddresseeLineEdit::loadContacts();

    const QString recentAddressGroupName = i18n( "Recent Addresses" );
    if ( MessageComposerSettings::self()->showRecentAddressesInComposer() ){
        const QStringList recent =
                KPIM::RecentAddresses::self( m_recentAddressConfig )->addresses();
        QStringList::ConstIterator it = recent.constBegin();
        QString name, email;

        KSharedConfig::Ptr config = KSharedConfig::openConfig( QLatin1String("kpimcompletionorder") );
        KConfigGroup group( config, "CompletionWeights" );
        const int weight = group.readEntry( "Recent Addresses", 10 );
        removeCompletionSource(recentAddressGroupName);
        const int idx = addCompletionSource( recentAddressGroupName, weight );

        QStringList::ConstIterator end = recent.constEnd();
        for ( ; it != end; ++it ) {
            KABC::Addressee addr;
            KPIMUtils::extractEmailAddressAndName( *it, email, name );
            name = KPIMUtils::quoteNameIfNecessary( name );
            if ( ( name[0] == QLatin1Char('"') ) && ( name[name.length() - 1] == QLatin1Char('"') ) ) {
                name.remove( 0, 1 );
                name.truncate( name.length() - 1 );
            }
            addr.setNameFromString( name );
            addr.insertEmail( email, true );
            addContact( addr, weight, idx );
        }
    } else {
        removeCompletionSource( recentAddressGroupName );
    }
}

void ComposerLineEdit::setRecentAddressConfig ( KConfig* config )
{
    m_recentAddressConfig = config;
}

bool ComposerLineEdit::expandIntern()
{
    return mExpandIntern;
}

void ComposerLineEdit::setExpandIntern(bool expand)
{
    mExpandIntern = expand;
}


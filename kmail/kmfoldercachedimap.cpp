/**
 *  kmfoldercachedimap.cpp
 *
 *  Copyright (c) 2002-2004 Bo Thorsen <bo@klaralvdalens-datakonsult.se>
 *  Copyright (c) 2002-2003 Steffen Hansen <steffen@klaralvdalens-datakonsult.se>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>

#include "kmkernel.h"
#include "kmfoldercachedimap.h"
#include "undostack.h"
#include "kmfoldermgr.h"
#include "kmmessage.h"
#include "kmacctcachedimap.h"
#include "kmacctmgr.h"
#include "kmailicalifaceimpl.h"
#include "kmfolder.h"
#include "kmdict.h"
#include "acljobs.h"
#include "kmbroadcaststatus.h"
#include "progressmanager.h"

using KMail::CachedImapJob;
using KMail::ImapAccountBase;

#include <kapplication.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kio/global.h>
#include <kio/scheduler.h>
#include <qbuffer.h>
#include <qfile.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qvaluelist.h>

#define UIDCACHE_VERSION 1


DImapTroubleShootDialog::DImapTroubleShootDialog( QWidget* parent,
                                                  const char* name )
  : KDialogBase( Plain, i18n( "Troubleshooting the IMAP cache" ),
                 Cancel | User1 | User2, Cancel, parent, name, true ),
    rc( Cancel )
{
  QFrame* page = plainPage();
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0 );
  QString txt = i18n( "<p><b>Troubleshooting the IMAP cache.</b></p>"
                      "<p>If you have problems with synchronizing an IMAP "
                      "folder, you should first try rebuilding the index "
                      "file. This will take some time to rebuild, but will "
                      "not cause any problems.</p><p>If that is not enough, "
                      "you can try refreshing the IMAP cache. If you do this, "
                      "you will loose all your local changes for this folder "
                      "and all it's subfolders.</p>" );
  topLayout->addWidget( new QLabel( txt, page ) );
  enableButtonSeparator( true );

  setButtonText( User1, i18n( "Refresh &Cache" ) );
  setButtonText( User2, i18n( "Rebuild &Index" ) );

  connect( this, SIGNAL( user1Clicked () ), this, SLOT( slotRebuildCache() ) );
  connect( this, SIGNAL( user2Clicked () ), this, SLOT( slotRebuildIndex() ) );
}

int DImapTroubleShootDialog::run()
{
  DImapTroubleShootDialog d;
  d.exec();
  return d.rc;
}

void DImapTroubleShootDialog::slotRebuildCache()
{
  rc = User1;
  done( User1 );
}

void DImapTroubleShootDialog::slotRebuildIndex()
{
  rc = User2;
  done( User2 );
}


KMFolderCachedImap::KMFolderCachedImap( KMFolder* folder, const char* aName )
  : KMFolderMaildir( folder, aName ),
    mSyncState( SYNC_STATE_INITIAL ), mContentState( imapNoInformation ),
    mSubfolderState( imapNoInformation ), mIsSelected( false ),
    mCheckFlags( true ), mAccount( NULL ), uidMapDirty( true ),
    mLastUid( 0 ), uidWriteTimer( -1 ), mUserRights( 0 ),
    mFolderRemoved( false ), mResync( false ),
    /*mHoldSyncs( false ),*/ mRecurse( true )
{
  setUidValidity("");
  mLastUid=0;
  readUidCache();

  mProgress = 0;
}

KMFolderCachedImap::~KMFolderCachedImap()
{
  if( !mFolderRemoved ) {
    // Only write configuration when the folder haven't been deleted
    KConfig* config = KMKernel::config();
    KConfigGroupSaver saver(config, "Folder-" + folder()->idString());
    config->writeEntry("ImapPath", mImapPath);
    config->writeEntry("NoContent", mNoContent);
    config->writeEntry("ReadOnly", mReadOnly);

    writeUidCache();
  }

  if (kmkernel->undoStack()) kmkernel->undoStack()->folderDestroyed( folder() );
}

void KMFolderCachedImap::initializeFrom( KMFolderCachedImap* parent )
{
  setAccount( parent->account() );
  // Now that we have an account, tell it that this folder was created:
  // if this folder was just removed, then we don't really want to remove it from the server.
  mAccount->removeDeletedFolder( imapPath() );
  setUserRights( parent->userRights() );
}

void KMFolderCachedImap::readConfig()
{
  KConfig* config = KMKernel::config();
  KConfigGroupSaver saver( config, "Folder-" + folder()->idString() );
  if( mImapPath.isEmpty() ) mImapPath = config->readEntry( "ImapPath" );
  if( QString( name() ).upper() == "INBOX" && mImapPath == "/INBOX/" )
  {
    folder()->setLabel( i18n( "inbox" ) );
    // for the icon
    folder()->setSystemFolder( true );
  }
  mNoContent = config->readBoolEntry( "NoContent", false );
  mReadOnly = config->readBoolEntry( "ReadOnly", false );

  KMFolderMaildir::readConfig();
}

void KMFolderCachedImap::remove()
{
  mFolderRemoved = true;

  QString part1 = folder()->path() + "/." + dotEscape(name());
  QString uidCacheFile = part1 + ".uidcache";
  // This is the account folder of an account that was just removed
  // When this happens, be sure to delete all traces of the cache
  if( QFile::exists(uidCacheFile) )
    unlink( QFile::encodeName( uidCacheFile ) );
  KIO::del( KURL::fromPathOrURL( part1 + ".directory" ) );

  // Tell the account (see listDirectory2)
  if (mAccount)
    mAccount->addDeletedFolder( imapPath() );

  FolderStorage::remove();
}

QString KMFolderCachedImap::uidCacheLocation() const
{
  QString sLocation(folder()->path());
  if (!sLocation.isEmpty()) sLocation += '/';
  return sLocation + '.' + dotEscape(fileName()) + ".uidcache";
}

int KMFolderCachedImap::readUidCache()
{
  QFile uidcache( uidCacheLocation() );
  if( uidcache.open( IO_ReadOnly ) ) {
    char buf[1024];
    int len = uidcache.readLine( buf, sizeof(buf) );
    if( len > 0 ) {
      int cacheVersion;
      sscanf( buf, "# KMail-UidCache V%d\n",  &cacheVersion );
      if( cacheVersion == UIDCACHE_VERSION ) {
        len = uidcache.readLine( buf, sizeof(buf) );
        if( len > 0 ) {
          setUidValidity( QString::fromLocal8Bit( buf).stripWhiteSpace() );
          len = uidcache.readLine( buf, sizeof(buf) );
          if( len > 0 ) {
            mLastUid =
              QString::fromLocal8Bit( buf).stripWhiteSpace().toULong();
            return 0;
          }
        }
      }
    }
  }
  return -1;
}

int KMFolderCachedImap::writeUidCache()
{
  if( uidValidity().isEmpty() || uidValidity() == "INVALID" ) {
    // No info from the server yet, remove the file.
    if( QFile::exists( uidCacheLocation() ) )
      unlink( QFile::encodeName( uidCacheLocation() ) );
    return 0;
  }

  QFile uidcache( uidCacheLocation() );
  if( uidcache.open( IO_WriteOnly ) ) {
    QTextStream str( &uidcache );
    str << "# KMail-UidCache V" << UIDCACHE_VERSION << endl;
    str << uidValidity() << endl;
    str << lastUid() << endl;
    uidcache.flush();
    fsync( uidcache.handle() ); /* this is probably overkill */
    uidcache.close();
    return 0;
  } else {
    return errno; /* does QFile set errno? */
  }
}

void KMFolderCachedImap::reloadUidMap()
{
  uidMap.clear();
  open();
  for( int i = 0; i < count(); ++i ) {
    KMMsgBase *msg = getMsgBase( i );
    if( !msg ) continue;
    ulong uid = msg->UID();
    uidMap.insert( uid, i );
    if( uid > mLastUid ) setLastUid( uid );
  }
  close();
  uidMapDirty = false;
}

/* Reimplemented from KMFolderMaildir */
KMMessage* KMFolderCachedImap::take(int idx)
{
  uidMapDirty = true;
  return KMFolderMaildir::take(idx);
}

// Add a message without clearing it's X-UID field.
int KMFolderCachedImap::addMsgInternal( KMMessage* msg, bool newMail,
                                        int* index_return )
{
  // Possible optimization: Only dirty if not filtered below
  ulong uid = msg->UID();
  if( uid != 0 ) {
    uidMapDirty = true;
    if( uid > mLastUid )
      setLastUid( uid );
  }

  // Add the message
  int rc = KMFolderMaildir::addMsg(msg, index_return);

  if( newMail && imapPath() == "/INBOX/" )
    // This is a new message. Filter it
    mAccount->processNewMsg( msg );

  return rc;
}

/* Reimplemented from KMFolderMaildir */
int KMFolderCachedImap::addMsg(KMMessage* msg, int* index_return)
{
  // Strip the IMAP UID
  msg->removeHeaderField( "X-UID" );
  msg->setUID( 0 );

  // Add it to storage
  return addMsgInternal( msg, false, index_return );
}


/* Reimplemented from KMFolderMaildir */
void KMFolderCachedImap::removeMsg(int idx, bool imapQuiet)
{
  uidMapDirty = true;
  // Remove it from disk
  KMFolderMaildir::removeMsg(idx,imapQuiet);
}

bool KMFolderCachedImap::canRemoveFolder() const {
  // If this has subfolders it can't be removed
  if( folder() && folder()->child() && folder()->child()->count() > 0 )
    return false;

#if 0
  // No special condition here, so let base class decide
  return KMFolderMaildir::canRemoveFolder();
#endif
  return true;
}

/* Reimplemented from KMFolderDir */
int KMFolderCachedImap::rename( const QString& aName,
                                KMFolderDir* /*aParent*/ )
{
  if ( aName == name() )
    // Stupid user trying to rename it to it's old name :)
    return 0;

  if( mSyncState != SYNC_STATE_INITIAL ) {
    KMessageBox::error( 0, i18n("You cannot rename a folder whilst a sync is in progress") );
    return -1;
  }

  if( account() == 0 ) {
    QString err = i18n("You must synchronize with the server before renaming IMAP folders.");
    KMessageBox::error( 0, err );
    return -1;
  }

  CachedImapJob *job = new CachedImapJob( aName, CachedImapJob::tRenameFolder, this );
  job->start();
  return 0;
}

KMFolder* KMFolderCachedImap::trashFolder() const
{
  QString trashStr = account()->trash();
  return kmkernel->dimapFolderMgr()->findIdString( trashStr );
}

void KMFolderCachedImap::setLastUid( ulong uid )
{
  mLastUid = uid;
  if( uidWriteTimer == -1 )
    // Write in one minute
    uidWriteTimer = startTimer( 60000 );
}

void KMFolderCachedImap::timerEvent( QTimerEvent* )
{
  killTimer( uidWriteTimer );
  uidWriteTimer = -1;
  writeUidCache();
}

ulong KMFolderCachedImap::lastUid()
{
  return mLastUid;
}

KMMsgBase* KMFolderCachedImap::findByUID( ulong uid )
{
  bool mapReloaded = false;
  if( uidMapDirty ) {
    reloadUidMap();
    mapReloaded = true;
  }

  QMap<ulong,int>::Iterator it = uidMap.find( uid );
  if( it != uidMap.end() ) {
    KMMsgBase *msg = getMsgBase( *it );
    if( msg && msg->UID() == uid )
      return msg;
  }
  // Not found by now
  if( mapReloaded )
    // Not here then
    return 0;
  // There could be a problem in the maps. Rebuild them and try again
  reloadUidMap();
  it = uidMap.find( uid );
  if( it != uidMap.end() )
    // Since the uid map is just rebuilt, no need for the sanity check
    return getMsg( *it );
  // Then it's not here
  return 0;
}

// This finds and sets the proper account for this folder if it has
// not been done
KMAcctCachedImap *KMFolderCachedImap::account() const
{
  if( (KMAcctCachedImap *)mAccount == 0 ) {
    // Find the account
    mAccount = static_cast<KMAcctCachedImap *>( kmkernel->acctMgr()->findByName( name() ) );
  }

  return mAccount;
}

void KMFolderCachedImap::slotTroubleshoot()
{
  const int rc = DImapTroubleShootDialog::run();

  if( rc == KDialogBase::User1 ) {
    // Refresh cache
    if( !account() ) {
      KMessageBox::sorry( 0, i18n("No account setup for this folder.\n"
                                  "Please try running a sync before this.") );
      return;
    }
    QString str = i18n("Are you sure you want to refresh the IMAP cache of "
                       "the folder %1 and all it's subfolders?\nThis will "
                       "remove all changes you have done locally to your "
                       "folders").arg( label() );
    QString s1 = i18n("Refresh IMAP Cache");
    QString s2 = i18n("&Refresh");
    if( KMessageBox::warningContinueCancel( 0, str, s1, s2 ) ==
        KMessageBox::Continue )
      account()->invalidateIMAPFolders( this );
  } else if( rc == KDialogBase::User2 ) {
    // Rebuild index file
    createIndexFromContents();
    KMessageBox::information( 0, i18n( "The index of this folder has been "
                                       "recreated." ) );
  }
}

void KMFolderCachedImap::serverSync( bool recurse )
{
  if( mSyncState != SYNC_STATE_INITIAL ) {
    if( KMessageBox::warningYesNo( 0, i18n("Folder %1 is not in initial sync state (state was %2). Do you want to reset it to initial sync state and sync anyway?" ).arg( imapPath() ).arg( mSyncState ) ) == KMessageBox::Yes ) {
      mSyncState = SYNC_STATE_INITIAL;
    } else return;
  }

  mRecurse = recurse;
  assert( account() );

  mAccount->mailCheckProgressItem()->reset();
  mAccount->mailCheckProgressItem()->setTotalItems( 100 );
  mProgress = 0;

#if 0
  if( mHoldSyncs ) {
    // All done for this folder.
    account()->mailCheckProgressItem()->setProgress( 100 );
    mProgress = 100; // all done
    newState( mProgress, i18n("Synchronization skipped"));
    mSyncState = SYNC_STATE_INITIAL;
    emit folderComplete( this, true );
    return;
  }
#endif

  mResync = false;
  serverSyncInternal();
}

QString KMFolderCachedImap::state2String( int state ) const
{
  switch( state ) {
  case SYNC_STATE_INITIAL:           return "SYNC_STATE_INITIAL";
  case SYNC_STATE_PUT_MESSAGES:      return "SYNC_STATE_PUT_MESSAGES";
  case SYNC_STATE_UPLOAD_FLAGS:      return "SYNC_STATE_UPLOAD_FLAGS";
  case SYNC_STATE_CREATE_SUBFOLDERS: return "SYNC_STATE_CREATE_SUBFOLDERS";
  case SYNC_STATE_LIST_SUBFOLDERS:   return "SYNC_STATE_LIST_SUBFOLDERS";
  case SYNC_STATE_LIST_SUBFOLDERS2:  return "SYNC_STATE_LIST_SUBFOLDERS2";
  case SYNC_STATE_DELETE_SUBFOLDERS: return "SYNC_STATE_DELETE_SUBFOLDERS";
  case SYNC_STATE_LIST_MESSAGES:     return "SYNC_STATE_LIST_MESSAGES";
  case SYNC_STATE_DELETE_MESSAGES:   return "SYNC_STATE_DELETE_MESSAGES";
  case SYNC_STATE_GET_MESSAGES:      return "SYNC_STATE_GET_MESSAGES";
  case SYNC_STATE_EXPUNGE_MESSAGES:  return "SYNC_STATE_EXPUNGE_MESSAGES";
  case SYNC_STATE_HANDLE_INBOX:      return "SYNC_STATE_HANDLE_INBOX";
  case SYNC_STATE_GET_USERRIGHTS:    return "SYNC_STATE_GET_USERRIGHTS";
  case SYNC_STATE_GET_ACLS:          return "SYNC_STATE_GET_ACLS";
  case SYNC_STATE_SET_ACLS:          return "SYNC_STATE_SET_ACLS";
  case SYNC_STATE_FIND_SUBFOLDERS:   return "SYNC_STATE_FIND_SUBFOLDERS";
  case SYNC_STATE_SYNC_SUBFOLDERS:   return "SYNC_STATE_SYNC_SUBFOLDERS";
  case SYNC_STATE_CHECK_UIDVALIDITY: return "SYNC_STATE_CHECK_UIDVALIDITY";
  default:                           return "Unknown state";
  }
}

/*
  Progress calculation: each step is assigned a span. Initially the total is 100.
  But if we skip a step, don't increase the progress.
  This leaves more room for the step a with variable size (get_messages)
   connecting 5
   getuserrights 5
   check_uidvalidity 5
   create_subfolders 5
   put_messages 10 (but it can take a very long time, with many messages....)
   upload_flags 5
   list_subfolders 5
   list_subfolders2 0 (all local)
   delete_subfolders 5
   list_messages 10
   delete_messages 10
   expunge_messages 5
   get_messages variable (remaining-5) i.e. minimum 20.
   set_acls 0 (rare)
   get_acls 5

  noContent folders have only a few of the above steps
  (permissions, and all subfolder stuff), so its steps should be given more span

 */

// While the server synchronization is running, mSyncState will hold
// the state that should be executed next
void KMFolderCachedImap::serverSyncInternal()
{
  // This is used to stop processing when we're about to exit
  // and the current job wasn't cancellable.
  // For user-requested abort, we'll use signalAbortRequested instead.
  if( KMBroadcastStatus::instance()->abortRequested() ) {
    resetSyncState();
    emit folderComplete( this, false );
    return;
  }

  //kdDebug(5006) << label() << ": " << state2String( mSyncState ) << endl;
  switch( mSyncState ) {
  case SYNC_STATE_INITIAL:
  {
    mProgress = 0;
    newState( mProgress, i18n("Synchronizing"));

    open();
    if ( !noContent() )
        mAccount->addLastUnreadMsgCount( countUnread() );

    // Connect to the server (i.e. prepare the slave)
    ImapAccountBase::ConnectionState cs = mAccount->makeConnection();
    if ( cs == ImapAccountBase::Error ) {
      // Cancelled by user, or slave can't start
      // kdDebug(5006) << "makeConnection said Error, aborting." << endl;
      // We stop here. We're already in SYNC_STATE_INITIAL for the next time.
      newState( mProgress, i18n( "Error connecting to server %1" ).arg( mAccount->host() ) );
      close();
      emit folderComplete(this, FALSE);
      break;
    } else if ( cs == ImapAccountBase::Connecting ) {
      // kdDebug(5006) << "makeConnection said Connecting, waiting for signal." << endl;
      newState( mProgress, i18n("Connecting to %1").arg( mAccount->host() ) );
      // We'll wait for the connectionResult signal from the account.
      connect( mAccount, SIGNAL( connectionResult(int, const QString&) ),
               this, SLOT( slotConnectionResult(int, const QString&) ) );
      break;
    } else {
      // Connected
      // kdDebug(5006) << "makeConnection said Connected, proceeding." << endl;
      mSyncState = SYNC_STATE_GET_USERRIGHTS;
      // Fall through to next state
    }
  }

  case SYNC_STATE_GET_USERRIGHTS:
    mSyncState = SYNC_STATE_CHECK_UIDVALIDITY;

    if( !noContent() && mAccount->hasACLSupport() ) {
      // Check the user's own rights. We do this every time in case they changed.
      newState( mProgress, i18n("Checking permissions"));
      connect( mAccount, SIGNAL( receivedUserRights( KMFolder* ) ),
               this, SLOT( slotReceivedUserRights( KMFolder* ) ) );
      mAccount->getUserRights( folder(), imapPath() ); // after connecting, due to the INBOX case
      break;
    }

  case SYNC_STATE_CHECK_UIDVALIDITY:
    emit syncRunning( folder(), true );
    mSyncState = SYNC_STATE_CREATE_SUBFOLDERS;
    if( !noContent() ) {
      checkUidValidity();
      break;
    }
    // Else carry on

  case SYNC_STATE_CREATE_SUBFOLDERS:
    mSyncState = SYNC_STATE_PUT_MESSAGES;
    createNewFolders();
    break;

  case SYNC_STATE_PUT_MESSAGES:
    mSyncState = SYNC_STATE_UPLOAD_FLAGS;
    if( !noContent() ) {
      uploadNewMessages();
      break;
    }
    // Else carry on
  case SYNC_STATE_UPLOAD_FLAGS:
    mSyncState = SYNC_STATE_LIST_SUBFOLDERS;
    if( !noContent() ) {
       // We haven't downloaded messages yet, so we need to build the map.
       if( uidMapDirty )
         reloadUidMap();
       uploadFlags();
      break;
    }
    // Else carry on
  case SYNC_STATE_LIST_SUBFOLDERS:
    mSyncState = SYNC_STATE_LIST_SUBFOLDERS2;
    newState( mProgress, i18n("Retrieving folderlist"));
    if( !listDirectory() ) {
      mSyncState = SYNC_STATE_INITIAL;
      KMessageBox::error(0, i18n("Error while retrieving the folderlist"));
    }
    break;

  case SYNC_STATE_LIST_SUBFOLDERS2:
    mSyncState = SYNC_STATE_DELETE_SUBFOLDERS;
    mProgress += 10;
    newState( mProgress, i18n("Retrieving subfolders"));
    listDirectory2();
    break;

  case SYNC_STATE_DELETE_SUBFOLDERS:
    mSyncState = SYNC_STATE_LIST_MESSAGES;
    if( !foldersForDeletionOnServer.isEmpty() ) {
      newState( mProgress, i18n("Deleting folders from server"));
      CachedImapJob* job = new CachedImapJob( foldersForDeletionOnServer,
                                                  CachedImapJob::tDeleteFolders, this );
      connect( job, SIGNAL( result(KMail::FolderJob *) ), this, SLOT( slotIncreaseProgress() ) );
      connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
      job->start();
      break;
    }
    // Not needed, the next step emits newState very quick
    //newState( mProgress, i18n("No folders to delete from server"));
      // Carry on

  case SYNC_STATE_LIST_MESSAGES:
    mSyncState = SYNC_STATE_DELETE_MESSAGES;
    if( !noContent() ) {
      newState( mProgress, i18n("Retrieving message list"));
      listMessages();
      break;
    }
    // Else carry on

  case SYNC_STATE_DELETE_MESSAGES:
    mSyncState = SYNC_STATE_EXPUNGE_MESSAGES;
    if( !noContent() ) {
      if( deleteMessages() ) {
        // Fine, we will continue with the next state
      } else {
        // No messages to delete, skip to GET_MESSAGES
        newState( mProgress, i18n("No messages to delete..."));
        mSyncState = SYNC_STATE_GET_MESSAGES;
        serverSyncInternal();
      }
      break;
    }
    // Else carry on

  case SYNC_STATE_EXPUNGE_MESSAGES:
    mSyncState = SYNC_STATE_GET_MESSAGES;
    if( !noContent() ) {
      newState( mProgress, i18n("Expunging deleted messages"));
      CachedImapJob *job = new CachedImapJob( QString::null,
                                              CachedImapJob::tExpungeFolder, this );
      connect( job, SIGNAL( result(KMail::FolderJob *) ), this, SLOT( slotIncreaseProgress() ) );
      connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
      job->start();
      break;
    }
    // Else carry on

  case SYNC_STATE_GET_MESSAGES:
    mSyncState = SYNC_STATE_HANDLE_INBOX;
    if( !noContent() ) {
      if( !mMsgsForDownload.isEmpty() ) {
        newState( mProgress, i18n("Retrieving new messages"));
        CachedImapJob *job = new CachedImapJob( mMsgsForDownload,
                                                CachedImapJob::tGetMessage,
                                                this );
        connect( job, SIGNAL( progress(unsigned long, unsigned long) ),
                 this, SLOT( slotProgress(unsigned long, unsigned long) ) );
        connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
        job->start();
        mMsgsForDownload.clear();
        break;
      } else {
        newState( mProgress, i18n("No new messages from server"));
        if( mLastUid == 0 && uidWriteTimer == -1 )
          // This is probably a new and empty folder. Write the UID cache
          writeUidCache();
      }
    }
    // Else carry on

  case SYNC_STATE_HANDLE_INBOX:
    // Wrap up the 'download emails' stage. We always end up at 95 here.
    mProgress = 95;

    if( mResync ) {
      // Some conflict have been resolved, so restart the sync
      mResync = false;
      mSyncState = SYNC_STATE_INITIAL;
      serverSyncInternal();
      break;
    } else
      // Continue with the ACLs
      mSyncState = SYNC_STATE_SET_ACLS;

  case SYNC_STATE_SET_ACLS:
    mSyncState = SYNC_STATE_GET_ACLS;

    if( !noContent() && mAccount->hasACLSupport() ) {
      bool hasChangedACLs = false;
      ACLList::ConstIterator it = mACLList.begin();
      for ( ; it != mACLList.end() && !hasChangedACLs; ++it ) {
        hasChangedACLs = (*it).changed;
      }
      if ( hasChangedACLs ) {
        newState( mProgress, i18n("Setting permissions"));
        KURL url = mAccount->getUrl();
        url.setPath( imapPath() );
        KIO::Job* job = KMail::ACLJobs::multiSetACL( mAccount->slave(), url, mACLList );
        ImapAccountBase::jobData jd( url.url(), folder() );
        mAccount->insertJob(job, jd);

        connect(job, SIGNAL(result(KIO::Job *)),
                SLOT(slotMultiSetACLResult(KIO::Job *)));
        connect(job, SIGNAL(aclChanged( const QString&, int )),
                SLOT(slotACLChanged( const QString&, int )) );
        break;
      }
    }

  case SYNC_STATE_GET_ACLS:
    // Continue with the subfolders
    mSyncState = SYNC_STATE_FIND_SUBFOLDERS;

    if( !noContent() && mAccount->hasACLSupport() ) {
      mAccount->getACL( folder(), mImapPath );
      connect( mAccount, SIGNAL(receivedACL( KMFolder*, KIO::Job*, const KMail::ACLList& )),
               this, SLOT(slotReceivedACL( KMFolder*, KIO::Job*, const KMail::ACLList& )) );
      break;
    }

  case SYNC_STATE_FIND_SUBFOLDERS:
    {
      mProgress = 98;
      newState( mProgress, i18n("Updating cache file"));

      mSyncState = SYNC_STATE_SYNC_SUBFOLDERS;
      mSubfoldersForSync.clear();
      mCurrentSubfolder = 0;
      if( folder() && folder()->child() ) {
        KMFolderNode *node = folder()->child()->first();
        while( node ) {
          if( !node->isDir() ) {
            if ( !static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage())->imapPath().isEmpty() )
              // Only sync folders that have been accepted by the server
              mSubfoldersForSync << static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage());
          }
          node = folder()->child()->next();
        }
      }
    }

    // All done for this folder.
    mProgress = 100; // all done
    newState( mProgress, i18n("Synchronization done"));
    emit syncRunning( folder(), false );

    if ( !mRecurse ) // "check mail for this folder" only
      mSubfoldersForSync.clear();

    // Carry on
  case SYNC_STATE_SYNC_SUBFOLDERS:
    {
      if( mCurrentSubfolder ) {
        disconnect( mCurrentSubfolder, SIGNAL( folderComplete(KMFolderCachedImap*, bool) ),
                    this, SLOT( slotSubFolderComplete(KMFolderCachedImap*, bool) ) );
        mCurrentSubfolder = 0;
      }

      if( mSubfoldersForSync.isEmpty() ) {
        mSyncState = SYNC_STATE_INITIAL;
        mAccount->addUnreadMsgCount( countUnread() ); // before closing
        close();
        emit folderComplete( this, TRUE );
      } else {
        mCurrentSubfolder = mSubfoldersForSync.front();
        mSubfoldersForSync.pop_front();
        connect( mCurrentSubfolder, SIGNAL( folderComplete(KMFolderCachedImap*, bool) ),
                 this, SLOT( slotSubFolderComplete(KMFolderCachedImap*, bool) ) );

        // kdDebug(5006) << "Sync'ing subfolder " << mCurrentSubfolder->imapPath() << endl;
        assert( !mCurrentSubfolder->imapPath().isEmpty() );
        mCurrentSubfolder->setAccount( account() );
        mCurrentSubfolder->serverSync( mRecurse /*which is true*/ );
      }
    }
    break;

  default:
    kdDebug(5006) << "KMFolderCachedImap::serverSyncInternal() WARNING: no such state "
              << mSyncState << endl;
  }
}

/* Connected to the imap account's connectionResult signal.
   Emitted when the slave connected or failed to connect.
*/
void KMFolderCachedImap::slotConnectionResult( int errorCode, const QString& errorMsg )
{
  disconnect( mAccount, SIGNAL( connectionResult(int, const QString&) ),
              this, SLOT( slotConnectionResult(int, const QString&) ) );
  if ( !errorCode ) {
    // Success
    mSyncState = SYNC_STATE_GET_USERRIGHTS;
    mProgress += 5;
    serverSyncInternal();
  } else {
    // Error (error message already shown by the account)
    newState( mProgress, KIO::buildErrorString( errorCode, errorMsg ));
    emit folderComplete(this, FALSE);
  }
}

/* find new messages (messages without a UID) */
QValueList<unsigned long> KMFolderCachedImap::findNewMessages()
{
  QValueList<unsigned long> result;
  for( int i = 0; i < count(); ++i ) {
    KMMsgBase *msg = getMsgBase( i );
    if( !msg ) continue; /* what goes on if getMsg() returns 0? */
    if ( msg->UID() == 0 )
      result.append( msg->getMsgSerNum() );
  }
  return result;
}

/* Upload new messages to server */
void KMFolderCachedImap::uploadNewMessages()
{
  QValueList<unsigned long> newMsgs = findNewMessages();
  if( !newMsgs.isEmpty() ) {

    newState( mProgress, i18n("Uploading messages to server"));
    CachedImapJob *job = new CachedImapJob( newMsgs, CachedImapJob::tPutMessage, this );
    connect( job, SIGNAL( progress( unsigned long, unsigned long) ),
             this, SLOT( slotPutProgress(unsigned long, unsigned long) ) );
    connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
    job->start();
  } else {
    newState( mProgress, i18n("No messages to upload to server"));

    serverSyncInternal();
  }
}

/* Progress info during uploadNewMessages */
void KMFolderCachedImap::slotPutProgress( unsigned long done, unsigned long total )
{
  // (going from mProgress to mProgress+10)
  int progressSpan = 10;
  newState( mProgress + (progressSpan * done) / total, QString::null );
  if ( done == total ) // we're done
    mProgress += progressSpan;
}

/* Upload message flags to server */
void KMFolderCachedImap::uploadFlags()
{
  if ( !uidMap.isEmpty() ) {
    mStatusFlagsJobs = 0;
    newState( mProgress, i18n("Uploading status of messages to server"));

    // FIXME DUPLICATED FROM KMFOLDERIMAP
    QMap< QString, QStringList > groups;
    //open(); //already done
    for( int i = 0; i < count(); ++i ) {
      KMMsgBase* msg = getMsgBase( i );
      if( !msg || msg->UID() == 0 )
        // Either not a valid message or not one that is on the server yet
        continue;

      QString flags = KMFolderImap::statusToFlags(msg->status());
      // Collect uids for each typem of flags.
      QString uid;
      uid.setNum( msg->UID() );
      groups[flags].append(uid);
    }
    QMapIterator< QString, QStringList > dit;
    for( dit = groups.begin(); dit != groups.end(); ++dit ) {
      QCString flags = dit.key().latin1();
      QStringList sets = KMFolderImap::makeSets( (*dit), true );
      mStatusFlagsJobs += sets.count(); // ### that's not in kmfolderimap....
      // Send off a status setting job for each set.
      for( QStringList::Iterator slit = sets.begin(); slit != sets.end(); ++slit ) {
        QString imappath = imapPath() + ";UID=" + ( *slit );
        mAccount->setImapStatus(folder(), imappath, flags);
      }
    }
    // FIXME END DUPLICATED FROM KMFOLDERIMAP

    if ( mStatusFlagsJobs ) {
      connect( mAccount, SIGNAL( imapStatusChanged(KMFolder*, const QString&, bool) ),
               this, SLOT( slotImapStatusChanged(KMFolder*, const QString&, bool) ) );
      return;
    }
  }
  newState( mProgress, i18n("No messages to upload to server"));
  serverSyncInternal();
}

void KMFolderCachedImap::slotImapStatusChanged(KMFolder* folder, const QString&, bool cont)
{
  if ( folder->storage() == this ) {
    --mStatusFlagsJobs;
    if ( mStatusFlagsJobs == 0 || !cont ) // done or aborting
      disconnect( mAccount, SIGNAL( imapStatusChanged(KMFolder*, const QString&, bool) ),
                  this, SLOT( slotImapStatusChanged(KMFolder*, const QString&, bool) ) );
    if ( mStatusFlagsJobs == 0 && cont ) {
      mProgress += 5;
      serverSyncInternal();
    }
  }
}


/* Upload new folders to server */
void KMFolderCachedImap::createNewFolders()
{
  QValueList<KMFolderCachedImap*> newFolders = findNewFolders();
  //kdDebug(5006) << label() << " createNewFolders:" << newFolders.count() << " new folders." << endl;
  if( !newFolders.isEmpty() ) {
    newState( mProgress, i18n("Creating subfolders on server"));
    CachedImapJob *job = new CachedImapJob( newFolders, CachedImapJob::tAddSubfolders, this );
    connect( job, SIGNAL( result(KMail::FolderJob *) ), this, SLOT( slotIncreaseProgress() ) );
    connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
    job->start();
  } else {
    serverSyncInternal();
  }
}

QValueList<KMFolderCachedImap*> KMFolderCachedImap::findNewFolders()
{
  QValueList<KMFolderCachedImap*> newFolders;
  if( folder() && folder()->child() ) {
    KMFolderNode *node = folder()->child()->first();
    while( node ) {
      if( !node->isDir() ) {
        if( static_cast<KMFolder*>(node)->folderType() != KMFolderTypeCachedImap ) {
          kdError(5006) << "KMFolderCachedImap::findNewFolders(): ARGH!!! "
                        << node->name() << " is not an IMAP folder\n";
          node = folder()->child()->next();
          assert(0);
        }
        KMFolderCachedImap* folder = static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage());
        if( folder->imapPath().isEmpty() ) newFolders << folder;
      }
      node = folder()->child()->next();
    }
  }
  return newFolders;
}

bool KMFolderCachedImap::deleteMessages()
{
  /* Delete messages from cache that are gone from the server */
  QPtrList<KMMessage> msgsForDeletion;

  // It is not possible to just go over all indices and remove
  // them one by one because the index list can get resized under
  // us. So use msg pointers instead

  QMap<ulong,int>::const_iterator it = uidMap.constBegin();
  for( ; it != uidMap.end(); it++ ) {
    ulong uid ( it.key() );
    if( uid!=0 && !uidsOnServer.find( uid ) )
      msgsForDeletion.append( getMsg( *it ) );
  }

  if( !msgsForDeletion.isEmpty() ) {
    removeMsg( msgsForDeletion );
  }

  /* Delete messages from the server that we dont have anymore */
  if( !uidsForDeletionOnServer.isEmpty() ) {
    newState( mProgress, i18n("Deleting removed messages from server"));
    QStringList sets = KMFolderImap::makeSets( uidsForDeletionOnServer, true );
    uidsForDeletionOnServer.clear();
    if( sets.count() > 1 ) {
      // Rerun the sync until the messages are all deleted
      mResync = true;
    }
    //kdDebug(5006) << "Deleting " << sets.front() << " from server folder " << imapPath() << endl;
    CachedImapJob *job = new CachedImapJob( sets.front(), CachedImapJob::tDeleteMessage,
                                                this );
    connect( job, SIGNAL( result(KMail::FolderJob *) ),
             this, SLOT( slotDeleteMessagesResult(KMail::FolderJob *) ) );
    job->start();
    return true;
  } else {
    return false;
  }
}

void KMFolderCachedImap::slotDeleteMessagesResult( KMail::FolderJob* job )
{
  if ( job->error() ) {
    // Skip the EXPUNGE state if deleting didn't work, no need to show two error messages
    mSyncState = SYNC_STATE_GET_MESSAGES;
  }
  mProgress += 10;
  serverSyncInternal();
}

void KMFolderCachedImap::checkUidValidity() {
  // IMAP root folders don't seem to have a UID validity setting.
  // Also, don't try the uid validity on new folders
  if( imapPath().isEmpty() || imapPath() == "/" )
    // Just proceed
    serverSyncInternal();
  else {
    newState( mProgress, i18n("Checking folder validity"));
    CachedImapJob *job = new CachedImapJob( FolderJob::tCheckUidValidity, this );
    // Increase progress by 5 when done.
    // We use result() to ensure it happens before serverSyncInternal :)
    connect( job, SIGNAL( result(KMail::FolderJob *) ), this, SLOT( slotIncreaseProgress() ) );
    connect( job, SIGNAL( finished() ), this, SLOT( serverSyncInternal() ) );
    job->start();
  }
}

/* This will only list the messages in a folder.
   No directory listing done*/
void KMFolderCachedImap::listMessages() {
  if( imapPath() == "/" ) {
    // Don't list messages on the root folder
    serverSyncInternal();
    return;
  }

  if( !mAccount->slave() ) { // sync aborted
    resetSyncState();
    emit folderComplete( this, false );
    return;
  }
  uidsOnServer.clear();
  uidsOnServer.resize( count() * 2 );
  uidsForDeletionOnServer.clear();
  mMsgsForDownload.clear();
  mUidsForDownload.clear();

  CachedImapJob* job = new CachedImapJob( FolderJob::tListMessages, this );
  connect( job, SIGNAL( result(KMail::FolderJob *) ),
           this, SLOT( slotGetLastMessagesResult(KMail::FolderJob *) ) );
  job->start();
}

void KMFolderCachedImap::slotGetLastMessagesResult(KMail::FolderJob *job)
{
  getMessagesResult(job, true);
}

// Connected to the listMessages job in CachedImapJob
void KMFolderCachedImap::slotGetMessagesData(KIO::Job * job, const QByteArray & data)
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) { // Shouldn't happen
    kdDebug(5006) << "could not find job!?!?!" << endl;
    serverSyncInternal(); /* HACK^W Fix: we should at least try to keep going */
    return;
  }
  (*it).cdata += QCString(data, data.size() + 1);
  int pos = (*it).cdata.find("\r\n--IMAPDIGEST");
  if (pos > 0) {
    int a = (*it).cdata.find("\r\nX-uidValidity:");
    if (a != -1) {
      int b = (*it).cdata.find("\r\n", a + 17);
      setUidValidity((*it).cdata.mid(a + 17, b - a - 17));
    }
    a = (*it).cdata.find("\r\nX-Access:");
    if (a != -1) {
      int b = (*it).cdata.find("\r\n", a + 12);
      QString access = (*it).cdata.mid(a + 12, b - a - 12);
      mReadOnly = access == "Read only";
    }
    (*it).cdata.remove(0, pos);
  }
  pos = (*it).cdata.find("\r\n--IMAPDIGEST", 1);
  // Start with something largish when rebuilding the cache
  if ( uidsOnServer.size() == 0 )
    uidsOnServer.resize( KMail::nextPrime( 2000 ) );
  int flags;
  const int v = 42;
  while (pos >= 0) {
    KMMessage msg;
    msg.fromString((*it).cdata.mid(16, pos - 16));
    flags = msg.headerField("X-Flags").toInt();
    ulong uid = msg.UID();
    if( uid != 0 ) {
      if ( uidsOnServer.count() == uidsOnServer.size() ) {
        uidsOnServer.resize( KMail::nextPrime( uidsOnServer.size() * 2 ) );
        kdDebug( 5006 ) << "Resizing to: " << uidsOnServer.size() << endl;
      }
      uidsOnServer.insert( uid, &v );
    }
    if ( /*flags & 8 ||*/ uid <= lastUid()) {
      /*
       * If this message UID is not present locally, then it must
       * have been deleted by the user, so we delete it on the
       * server also.
       *
       * This relies heavily on lastUid() being correct at all times.
       */
      // kdDebug(5006) << "KMFolderCachedImap::slotGetMessagesData() : folder "<<label()<<" already has msg="<<msg->headerField("Subject") << ", UID="<<uid << ", lastUid = " << mLastUid << endl;
      KMMsgBase *existingMessage = findByUID(uid);
      if( !existingMessage ) {
         // kdDebug(5006) << "message with uid " << uid << " is gone from local cache. Must be deleted on server!!!" << endl;
         uidsForDeletionOnServer << uid;
      } else {
         /* The message is OK, update flags */
         KMFolderImap::flagsToStatus( existingMessage, flags );
         // kdDebug(5006) << "message with uid " << uid << " found in the local cache. " << endl;
      }
    } else {
      ulong size = msg.headerField("X-Length").toULong();
      mMsgsForDownload << KMail::CachedImapJob::MsgForDownload(uid, flags, size);
      if( imapPath() == "/INBOX/" )
         mUidsForDownload << uid;

    }
    (*it).cdata.remove(0, pos);
    (*it).done++;
    pos = (*it).cdata.find("\r\n--IMAPDIGEST", 1);
  }
}

void KMFolderCachedImap::getMessagesResult( KMail::FolderJob *job, bool lastSet )
{
  mProgress += 10;
  if( job->error() ) { // error listing messages but the user chose to continue
    mContentState = imapNoInformation;
  } else {
    if( lastSet ) { // always true here (this comes from online-imap...)
      mContentState = imapFinished;
    }
  }
  serverSyncInternal();
}

void KMFolderCachedImap::slotProgress(unsigned long done, unsigned long total)
{
  int progressSpan = 100 - 5 - mProgress;
  //kdDebug(5006) << "KMFolderCachedImap::slotProgress done=" << done << " total=" << total << "=> mProgress=" << mProgress + ( progressSpan * done ) / total << endl;
  // Progress info while retrieving new emails
  // (going from mProgress to mProgress+progressSpan)
  newState( mProgress + (progressSpan * done) / total, QString::null );
}


void KMFolderCachedImap::setAccount(KMAcctCachedImap *aAccount)
{
  assert( aAccount->isA("KMAcctCachedImap") );
  mAccount = aAccount;
  if( imapPath()=="/" ) aAccount->setFolder( folder() );

  if( !folder() || !folder()->child() || !folder()->child()->count() ) return;
  for( KMFolderNode* node = folder()->child()->first(); node;
       node = folder()->child()->next() )
    if (!node->isDir())
      static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage())->setAccount(aAccount);
}


// This lists the subfolders on the server
// and (in slotListResult) takes care of folders that have been removed on the server
bool KMFolderCachedImap::listDirectory(bool secondStep)
{
  mSubfolderState = imapInProgress;
  if( !mAccount->slave() ) { // sync aborted
    resetSyncState();
    emit folderComplete( this, false );
    return false;
  }

  // connect to folderlisting
  connect(mAccount, SIGNAL(receivedFolders(QStringList, QStringList,
          QStringList, const ImapAccountBase::jobData &)),
      this, SLOT(slotListResult(QStringList, QStringList,
          QStringList, const ImapAccountBase::jobData &)));

  // start a new listing for the root-folder
  bool reset = ( mImapPath == mAccount->prefix() &&
                !secondStep && !folder()->isSystemFolder() ) ? true : false;

  // get the folders
  ImapAccountBase::ListType type =
    (mAccount->onlySubscribedFolders() ? ImapAccountBase::ListSubscribed : ImapAccountBase::List);
  mAccount->listDirectory(mImapPath, type,
                          secondStep, folder(), reset);

  return true;
}

void KMFolderCachedImap::slotListResult( QStringList folderNames,
                                         QStringList folderPaths,
                                         QStringList folderMimeTypes,
                                         const ImapAccountBase::jobData & jobData )
{
  //kdDebug(5006) << label() << ": folderNames=" << folderNames << " folderPaths=" << folderPaths << " mimeTypes=" << folderMimeTypes << endl;
  mSubfolderNames = folderNames;
  mSubfolderPaths = folderPaths;
  mSubfolderMimeTypes = folderMimeTypes;
  if (jobData.parent) {
    // the account is connected to several folders, so we
    // have to sort out if this result is for us
    if (jobData.parent != folder()) return;
  }
  // disconnect to avoid recursions
  disconnect(mAccount, SIGNAL(receivedFolders(QStringList, QStringList,
          QStringList, const ImapAccountBase::jobData &)),
      this, SLOT(slotListResult(QStringList, QStringList,
          QStringList, const ImapAccountBase::jobData &)));

  mSubfolderState = imapFinished;
  bool it_inboxOnly = jobData.inboxOnly;

  if (it_inboxOnly) {
    // list again only for the INBOX
    listDirectory(TRUE);
    return;
  }

  if ( folder()->isSystemFolder() && mImapPath == "/INBOX/"
       && mAccount->prefix() == "/INBOX/" )
  {
    // do not create folders under INBOX
    mAccount->setCreateInbox(FALSE);
    mSubfolderNames.clear();
  }
  folder()->createChildFolder();
  // Find all subfolders present on disk but not on the server
  KMFolderNode *node = folder()->child()->first();
  QPtrList<KMFolder> toRemove;
  while (node) {
    if (!node->isDir() ) {
      KMFolderCachedImap *f = static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage());
      if ( mSubfolderNames.findIndex(node->name()) == -1 &&
          (node->name().upper() != "INBOX" || !mAccount->createInbox()) )
      {
        // This subfolder isn't present on the server
        if( !f->imapPath().isEmpty()  ) {
          // The folder has an imap path set, so it has been
          // on the server before. Delete it locally.
          toRemove.append( f->folder() );
          kdDebug(5006) << node->name() << " isn't on the server. It has an imapPath -> delete it locally" << endl;
        } else { // shouldn't happen
          kdDebug(5006) << node->name() << " isn't on the server, but has no imapPath. ERROR - why didn't createNewFolders create it?" << endl;
        }
      } else { // folder both local and on server
        //kdDebug(5006) << node->name() << " is on the server." << endl;
      }
    } else {
      //kdDebug(5006) << "skipping dir node:" << node->name() << endl;
    }
    node = folder()->child()->next();
  }

  for ( KMFolder* doomed=toRemove.first(); doomed; doomed = toRemove.next() )
    kmkernel->dimapFolderMgr()->remove( doomed );

  mProgress += 5;
  serverSyncInternal();
}

// This synchronizes the local folders as needed (creation/deletion). No network communication here.
void KMFolderCachedImap::listDirectory2() {
  foldersForDeletionOnServer.clear();
  QString path = folder()->path();
  KMFolderCachedImap *f = 0;
  kmkernel->dimapFolderMgr()->quiet(true);

  if (mAccount->createInbox())
  {
    KMFolderNode *node;
    // create the INBOX
    for (node = folder()->child()->first(); node; node = folder()->child()->next())
      if (!node->isDir() && node->name() == "INBOX") break;
    if (node)
      f = static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage());
    else {
      KMFolder* newFolder = folder()->child()->createFolder("INBOX", true, KMFolderTypeCachedImap);
      if (newFolder)
        f = static_cast<KMFolderCachedImap*>(newFolder->storage());
    }
    f->setAccount(mAccount);
    f->setImapPath("/INBOX/");
    f->folder()->setLabel(i18n("inbox"));
    if (!node) {
      f->close();
      kmkernel->dimapFolderMgr()->contentsChanged();
    }
    // so we have an INBOX
    mAccount->setCreateInbox( false );
    mAccount->setHasInbox( true );
  }

  // Find all subfolders present on server but not on disk
  for (uint i = 0; i < mSubfolderNames.count(); i++) {

    if (mSubfolderNames[i].upper() == "INBOX" &&
        mSubfolderPaths[i] == "/INBOX/" &&
        mAccount->hasInbox()) // do not create an additional inbox
      continue;

    // Find the subdir, if already present
    KMFolderNode *node;
    for (node = folder()->child()->first(); node;
         node = folder()->child()->next())
      if (!node->isDir() && node->name() == mSubfolderNames[i]) break;

    if (!node) {
      // This folder is not present here
      // Either it's new on the server, or we just deleted it.
      QString subfolderPath = mSubfolderPaths[i];
      // The code used to look at the uidcache to know if it was "just deleted".
      // But this breaks with noContent folders and with shared folders.
      // So instead we keep a list in the account.
      bool locallyDeleted = mAccount->isDeletedFolder( subfolderPath );
      // That list is saved/restored across sessions, but to avoid any mistake,
      // ask for confirmation if the folder was deleted in a previous session
      // (could be that the folder was deleted & recreated meanwhile from another client...)
      if ( !locallyDeleted && mAccount->isPreviouslyDeletedFolder( subfolderPath ) ) {
           locallyDeleted = KMessageBox::warningYesNo(
             0, i18n( "<qt><p>It seems that the folder <b>%1</b> was deleted. Do you want to delete it from the server?</p></qt>" ).arg( mSubfolderNames[i] ) ) == KMessageBox::Yes;
      }

      if ( locallyDeleted ) {
        kdDebug(5006) << subfolderPath << " was deleted locally => delete on server." << endl;
        foldersForDeletionOnServer << subfolderPath;
      } else {
        kdDebug(5006) << subfolderPath << " is a new folder on the server => create local cache" << endl;
        KMFolder* newFolder = folder()->child()->createFolder(mSubfolderNames[i], false, KMFolderTypeCachedImap);
        if (newFolder)
          f = static_cast<KMFolderCachedImap*>(newFolder->storage());
        if (f) {
          f->close();
          f->setAccount(mAccount);
          kmkernel->dimapFolderMgr()->contentsChanged();
        } else {
          kdDebug(5006) << "can't create folder " << mSubfolderNames[i] <<endl;
        }
      }
    } else { // Folder found locally
      if( static_cast<KMFolder*>(node)->folderType() == KMFolderTypeCachedImap )
        f = static_cast<KMFolderCachedImap*>(static_cast<KMFolder*>(node)->storage());
    }

    if( f ) {
      // kdDebug(5006) << "folder("<<f->name()<<")->imapPath()=" << f->imapPath()
      //               << "\nSetting imapPath " << mSubfolderPaths[i] << endl;
      // Write folder settings
      f->setAccount(mAccount);
      f->setNoContent(mSubfolderMimeTypes[i] == "inode/directory");
      f->setNoChildren(mSubfolderMimeTypes[i] == "message/digest");
      f->setImapPath(mSubfolderPaths[i]);
    }
  }
  kmkernel->dimapFolderMgr()->quiet(false);
  emit listComplete(this);
  serverSyncInternal();
}

void KMFolderCachedImap::slotSubFolderComplete(KMFolderCachedImap* sub, bool success)
{
  Q_UNUSED(sub);
  //kdDebug(5006) << label() << " slotSubFolderComplete: " << sub->label() << endl;
  if ( success ) {
    serverSyncInternal();
  }
  else
  {
    // success == false means the sync was aborted.
    if ( mCurrentSubfolder ) {
      Q_ASSERT( sub == mCurrentSubfolder );
      disconnect( mCurrentSubfolder, SIGNAL( folderComplete(KMFolderCachedImap*, bool) ),
                  this, SLOT( slotSubFolderComplete(KMFolderCachedImap*, bool) ) );
      mCurrentSubfolder = 0;
    }

    mSubfoldersForSync.clear();
    mSyncState = SYNC_STATE_INITIAL;
    close();
    emit folderComplete( this, false );
  }
}

void KMFolderCachedImap::slotSimpleData(KIO::Job * job, const QByteArray & data)
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  if (it == mAccount->jobsEnd()) return;
  QBuffer buff((*it).data);
  buff.open(IO_WriteOnly | IO_Append);
  buff.writeBlock(data.data(), data.size());
  buff.close();
}


FolderJob*
KMFolderCachedImap::doCreateJob( KMMessage *msg, FolderJob::JobType jt, KMFolder *folder,
                                 QString, const AttachmentStrategy* ) const
{
  QPtrList<KMMessage> msgList;
  msgList.append( msg );
  CachedImapJob *job = new CachedImapJob( msgList, jt, folder? static_cast<KMFolderCachedImap*>( folder->storage() ):0 );
  job->setParentFolder( this );
  return job;
}

FolderJob*
KMFolderCachedImap::doCreateJob( QPtrList<KMMessage>& msgList, const QString& sets,
                                 FolderJob::JobType jt, KMFolder *folder ) const
{
  //FIXME: how to handle sets here?
  Q_UNUSED( sets );
  CachedImapJob *job = new CachedImapJob( msgList, jt, folder? static_cast<KMFolderCachedImap*>( folder->storage() ):0 );
  job->setParentFolder( this );
  return job;
}

void
KMFolderCachedImap::setUserRights( unsigned int userRights )
{
  mUserRights = userRights;
}

void
KMFolderCachedImap::slotReceivedUserRights( KMFolder* folder )
{
  if ( folder->storage() == this ) {
    disconnect( mAccount, SIGNAL( receivedUserRights( KMFolder* ) ),
                this, SLOT( slotReceivedUserRights( KMFolder* ) ) );
    if ( mUserRights == 0 ) // didn't work
      mUserRights = -1; // error code (used in folderdia)
    mProgress += 5;
    serverSyncInternal();
  }
}

void
KMFolderCachedImap::slotReceivedACL( KMFolder* folder, KIO::Job*, const KMail::ACLList& aclList )
{
  if ( folder->storage() == this ) {
    disconnect( mAccount, SIGNAL(receivedACL( KMFolder*, KIO::Job*, const KMail::ACLList& )),
                this, SLOT(slotReceivedACL( KMFolder*, KIO::Job*, const KMail::ACLList& )) );
    mACLList = aclList;
    serverSyncInternal();
  }
}

void
KMFolderCachedImap::setACLList( const ACLList& arr )
{
  mACLList = arr;
}

void
KMFolderCachedImap::slotMultiSetACLResult(KIO::Job *job)
{
  KMAcctCachedImap::JobIterator it = mAccount->findJob(job);
  if ( it == mAccount->jobsEnd() ) return; // Shouldn't happen
  if ( (*it).parent != folder() ) return; // Shouldn't happen

  if ( job->error() )
    // Display error but don't abort the sync just for this
    // PENDING(dfaure) reconsider using handleJobError now that it offers continue/cancel
    job->showErrorDialog();

  if (mAccount->slave()) mAccount->removeJob(job);
  serverSyncInternal();
}

void
KMFolderCachedImap::slotACLChanged( const QString& userId, int permissions )
{
  // The job indicates success in changing the permissions for this user
  // -> we note that it's been done.
  for( ACLList::Iterator it = mACLList.begin(); it != mACLList.end(); ++it ) {
    if ( (*it).userId == userId && (*it).permissions == permissions ) {
      if ( permissions == -1 ) // deleted
        mACLList.erase( it );
      else // added/modified
        (*it).changed = false;
      return;
    }
  }
}

// called by KMAcctCachedImap::killAllJobs
void KMFolderCachedImap::resetSyncState()
{
  mSubfoldersForSync.clear();
  mSyncState = SYNC_STATE_INITIAL;
  close();
  // Don't use newState here, it would revert to mProgress (which is < current value when listing messages)
  ProgressItem *progressItem = mAccount->mailCheckProgressItem();
  QString str = i18n("Aborted");
  progressItem->setStatus( str );
  emit statusMsg( str );
}

void KMFolderCachedImap::slotIncreaseProgress()
{
  mProgress += 5;
}

void KMFolderCachedImap::newState( int progress, const QString& syncStatus )
{
  //kdDebug() << k_funcinfo << folder() << " " << mProgress << " " << syncStatus << endl;
  ProgressItem *progressItem = mAccount->mailCheckProgressItem();
  progressItem->setCompletedItems( progress );
  if ( !syncStatus.isEmpty() ) {
    QString str;
    // For a subfolder, show the label. But for the main folder, it's already shown.
    if ( mAccount->imapFolder() == this )
      str = syncStatus;
    else
      str = QString( "%1: %2" ).arg( label() ).arg( syncStatus );
    progressItem->setStatus( str );
    emit statusMsg( str );
  }
  progressItem->updateProgress();
}

#include "kmfoldercachedimap.moc"

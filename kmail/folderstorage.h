/*
    Virtual base class for mail storage.

    This file is part of KMail.

    Copyright (c) 2004 Bo Thorsen <bo@klaralvdalens-datakonsult.se>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef FOLDERSTORAGE_H
#define FOLDERSTORAGE_H

// for large file support
#include <config.h>

#include "kmfoldernode.h"
#include "kmfoldertype.h"
#include "kmmsginfo.h"
#include "kmglobal.h"
#include "folderjob.h"
using KMail::FolderJob;

#include "mimelib/string.h"

#include <qptrvector.h>
#include <sys/types.h>
#include <stdio.h>

class KMMessage;
class KMFolderDir;
class KMAcctList;
class KMMsgDict;
class KMMsgDictREntry;
class QTimer;

namespace KMail {
   class AttachmentStrategy;
}
using KMail::AttachmentStrategy;

typedef QValueList<Q_UINT32> SerNumList;

/** Mail folder.
 * (description will be here).
 *
 * @sect Accounts:
 *   The accounts (of KMail) that are fed into the folder are
 *   represented as the children of the folder. They are only stored here
 *   during runtime to have a reference for which accounts point to a
 *   specific folder.
 */

class FolderStorage : public QObject
{
  Q_OBJECT

public:


  /** Usually a parent is given. But in some cases there is no
    fitting parent object available. Then the name of the folder
    is used as the absolute path to the folder file. */
  FolderStorage( KMFolder* folder, const char* name=0 );
  virtual ~FolderStorage();

  KMFolder* folder() const { return mFolder; }

  void setAcctList( KMAcctList* list ) { mAcctList = list; }
  KMAcctList* acctList() const { return mAcctList; }

  /** Returns the type of this folder */
  virtual KMFolderType folderType() const { return KMFolderTypeUnknown; }

  /** Returns the filename of the folder (reimplemented in KMFolderImap) */
  virtual QString fileName() const { return name(); }

  /** Returns full path to folder file */
  QString location() const;

  /** Returns full path to index file */
  virtual QString indexLocation() const = 0;

  /** Returns, if the folder can't contain mails, but only subfolder */
  virtual bool noContent() const { return mNoContent; }

  /** Specify, that the folder can't contain mails. */
  virtual void setNoContent(bool aNoContent)
    { mNoContent = aNoContent; }

  /** Read message at given index. Indexing starts at zero */
  virtual KMMessage* getMsg(int idx);

  /** Replace KMMessage with KMMsgInfo and delete KMMessage  */
  virtual KMMsgInfo* unGetMsg(int idx);

  /** Checks if the message is already "gotten" with getMsg */
  virtual bool isMessage(int idx);

  /** Read a message and return a referece to a string */
  virtual QCString& getMsgString(int idx, QCString& mDest) = 0;

  /** Read a message and returns a DwString */
  virtual DwString getDwString(int idx) = 0;

  /**
   * Removes and deletes all jobs associated with the particular message
   */
  virtual void ignoreJobsForMessage( KMMessage* );

  /**
   * These methods create respective FolderJob (You should derive FolderJob
   * for each derived KMFolder).
   */
  virtual FolderJob* createJob( KMMessage *msg, FolderJob::JobType jt = FolderJob::tGetMessage,
                                KMFolder *folder = 0, QString partSpecifier = QString::null, 
                                const AttachmentStrategy *as = 0 ) const;
  virtual FolderJob* createJob( QPtrList<KMMessage>& msgList, const QString& sets,
                                FolderJob::JobType jt = FolderJob::tGetMessage,
                                KMFolder *folder = 0 ) const;

  /** Provides access to the basic message fields that are also stored
    in the index. Whenever you only need subject, from, date, status
    you should use this method instead of getMsg() because getMsg()
    will load the message if necessary and this method does not. */
  virtual const KMMsgBase* getMsgBase(int idx) const = 0;
  virtual KMMsgBase* getMsgBase(int idx) = 0;

  /** Same as getMsgBase(int). */
  virtual const KMMsgBase* operator[](int idx) const { return getMsgBase(idx); }

  /** Same as getMsgBase(int). This time non-const. */
  virtual KMMsgBase* operator[](int idx) { return getMsgBase(idx); }

  /** Detach message from this folder. Usable to call addMsg() afterwards.
    Loads the message if it is not loaded up to now. */
  virtual KMMessage* take(int idx);
  virtual void take(QPtrList<KMMessage> msgList);

  /** Add the given message to the folder. Usually the message
    is added at the end of the folder. Returns zero on success and
    an errno error code on failure. The index of the new message
    is stored in index_return if given.
    Please note that the message is added as is to the folder and the folder
    takes ownership of the message (deleting it in the destructor).*/
  virtual int addMsg(KMMessage* msg, int* index_return = 0) = 0;

  /** (Note(bo): This needs to be fixed better at a later point.)
      This is overridden by dIMAP because addMsg strips the X-UID
      header from the mail. */
  virtual int addMsgKeepUID(KMMessage* msg, int* index_return = 0) {
    return addMsg(msg, index_return);
  }

  /** Called by derived classes implementation of addMsg.
      Emits msgAdded signals */
  void emitMsgAddedSignals(int idx);

  /** Returns FALSE, if the message has to be retrieved from an IMAP account
   * first. In this case this function does this and cares for the rest */
  virtual bool canAddMsgNow(KMMessage* aMsg, int* aIndex_ret);

  /** Remove (first occurrence of) given message from the folder. */
  virtual void removeMsg(int i, bool imapQuiet = FALSE);
  virtual void removeMsg(QPtrList<KMMessage> msgList, bool imapQuiet = FALSE);

  /** Delete messages in the folder that are older than days. Return the
   * number of deleted messages. */
  virtual int expungeOldMsg(int days);

  /** Detaches the given message from it's current folder and
    adds it to this folder. Returns zero on success and an errno error
    code on failure. The index of the new message is stored in index_return
    if given. */
  virtual int moveMsg(KMMessage* msg, int* index_return = 0);
  virtual int moveMsg(QPtrList<KMMessage>, int* index_return = 0);

  /** Returns the index of the given message or -1 if not found. */
  virtual int find(const KMMsgBase* msg) const = 0;
  int find( const KMMessage * msg ) const;

  /** Number of messages in this folder. */
  virtual int count(bool cache = false) const;

  /** Number of new or unread messages in this folder. */
  virtual int countUnread();

  /** Called by KMMsgBase::setStatus when status of a message has changed
      required to keep the number unread messages variable current. */
  virtual void msgStatusChanged( const KMMsgStatus oldStatus,
                                 const KMMsgStatus newStatus,
				 int idx);

  /** Open folder for access. Does not work if the parent is not set.
    Does nothing if the folder is already opened. To reopen a folder
    call close() first.
    Returns zero on success and an error code equal to the c-library
    fopen call otherwise (errno). */
  virtual int open() = 0;

  /** Check folder for permissions
    Returns zero if readable and writable. */
  virtual int canAccess() = 0;

  /** Close folder. If force is TRUE the files are closed even if
    others still use it (e.g. other mail reader windows). */
  virtual void close(bool force=FALSE) = 0;

  /** fsync buffers to disk */
  virtual void sync() = 0;

  /** Test if folder is opened. */
  bool isOpened() const { return (mOpenCount>0); }

  /** Mark all new messages as unread. */
  virtual void markNewAsUnread();

  /** Mark all new and unread messages as read. */
  virtual void markUnreadAsRead();

  /** Create a new folder with the name of this object and open it.
      Returns zero on success and an error code equal to the
      c-library fopen call otherwise. */
  virtual int create(bool imap = FALSE) = 0;

  /** Removes the folder physically from disk and empties the contents
    of the folder in memory. Note that the folder is closed during this
    process, whether there are others using it or not.
    @see KMFolder::removeContents */
  virtual int remove();

  /** Delete entire folder. Forces a close *but* opens the
    folder again afterwards. Returns errno(3) error code or zero on
    success.  @see KMFolder::expungeContents */
  virtual int expunge();

  /** Remove deleted messages from the folder. Returns zero on success
    and an errno on failure. */
  virtual int compact() = 0;

  /** Physically rename the folder. Returns zero on success and an errno
    on failure. */
  virtual int rename(const QString& newName, KMFolderDir *aParent = 0);

  /** Returns TRUE if a table of contents file is automatically created. */
  bool autoCreateIndex() const { return mAutoCreateIndex; }

  /** Allow/disallow automatic creation of a table of contents file.
    Default is TRUE. */
  virtual void setAutoCreateIndex(bool);

  /** Returns TRUE if the table of contents is dirty. This happens when
    a message is deleted from the folder. The toc will then be re-created
    when the folder is closed. */
  bool dirty() const { return mDirty; }

  /** Change the dirty flag. */
  void setDirty(bool f);

  /** Returns TRUE if the folder contains deleted messages */
  bool needsCompacting() const { return needsCompact; }
  virtual void setNeedsCompacting(bool f) { needsCompact = f; }

  /** If set to quiet the folder will not emit msgAdded(idx) signal.
    This is necessary because adding the messages to the listview
    one by one as they come in ( as happens on msgAdded(idx) ) is
    very slow for large ( >10000 ) folders. For pop, where whole
    bodies are downloaded this is not an issue, but for imap, where
    we only download headers it becomes a bottleneck. We therefore
    set the folder quiet() and rebuild the listview completely once
    the complete folder has been checked. */
  virtual void quiet(bool beQuiet);

  /** Is the folder read-only? */
  virtual bool isReadOnly() const = 0;

  
  /** Returns the label of the folder for visualization. */
  virtual QString label() const;
  void setLabel(const QString& lbl) { mLabel = lbl; }

  /** Type of the folder. Inherited. */
  virtual const char* type() const;

  virtual QCString protocol() const = 0;

  /** Returns TRUE if accounts are associated with this folder. */
  bool hasAccounts() const { return (mAcctList != 0); }

  /** Returns TRUE if this folder is associated with a mailing-list. */
  void setMailingList(bool enabled)
  { mMailingListEnabled = enabled; writeConfig(); }
  bool isMailingList() const { return mMailingListEnabled; }

  void setMailingListPostAddress(const QString &address)
  { mMailingListPostingAddress = address; writeConfig(); }
  QString mailingListPostAddress() const
  { return mMailingListPostingAddress; }

  void setMailingListAdminAddress(const QString &address)
  { mMailingListAdminAddress = address; writeConfig(); }
  QString mailingListAdminAddress() const
  { return mMailingListAdminAddress; }

  void setIdentity(uint identity);
  uint identity() const { return mIdentity; }

  /** Get / set the name of the field that is used for the Sender/Receiver column in the headers (From/To) */
  QString whoField() const { return mWhoField; }
  void setWhoField(const QString& aWhoField) { mWhoField = aWhoField; /*writeConfig();*/ }

  /** Get / set the user-settings for the WhoField (From/To/Empty) */
  QString userWhoField(void) { return mUserWhoField; }
  void setUserWhoField(const QString &whoField,bool writeConfig=true);

  /** A cludge to help make sure the count of unread messges is kept in sync */
  virtual void correctUnreadMsgsCount();

  /** Returns a string that can be used to identify this folder */
  virtual QString idString() const;

  /** Write index to index-file. Returns 0 on success and errno error on
    failure. */
  virtual int writeIndex( bool createEmptyIndex = false ) = 0;

  /** Inserts messages into the message dictionary.  Might be called
    during kernel initialization. */
  void fillMsgDict(KMMsgDict *dict);

  /** Writes the message serial number file. */
  int writeMsgDict(KMMsgDict *dict = 0);

  /** Touches the message serial number file. */
  int touchMsgDict();

  /** Append message to end of message serial number file. */
  int appendtoMsgDict(int idx = -1);

  /** Sets the reverse-dictionary for this folder. */
  void setRDict(KMMsgDictREntry *rentry);

  /** Returns the reverse-dictionary for this folder. */
  KMMsgDictREntry *rDict() const { return mRDict; }

  /** Set the status of the message at index @p idx to @p status. */
  virtual void setStatus(int idx, KMMsgStatus status, bool toggle=false);

  /** Set the status of the message(s) in the QValueList @p ids to @p status. */
  virtual void setStatus(QValueList<int>& ids, KMMsgStatus status, bool toggle=false);

  void removeJobs();

  /** Convert "\r\n" line endings in "\n" line endings. The conversion
      happens in place. Returns the length of the resulting string.
  */
  static size_t crlf2lf( char* str, const size_t strLen );

  /** Escape a leading dot */
  static QString dotEscape(const QString&);

  /** Write the config file */
  virtual void writeConfig();

signals:
  /** Emitted when the status, name, or associated accounts of this
    folder changed. */
  void changed();

  /** Emitted when the contents of a folder have been cleared
     (new search in a search folder, for example) */
  void cleared();

  /** Emitted after an expunge. If not quiet, changed() will be
      emmitted first. */
  void expunged();

  /** Emitted when the name of the folder changes. */
  void nameChanged();

  /** Emitted before a message is removed from the folder. */
  void msgRemoved(KMFolder*, Q_UINT32 sernum);

  /** Emitted after a message is removed from the folder. */
  void msgRemoved(int idx,QString msgIdMD5, QString strippedSubjMD5);
  void msgRemoved(KMFolder*);

  /** Emitted when a message is added from the folder. */
  void msgAdded(int idx);
  void msgAdded(KMFolder*, Q_UINT32 sernum);

  /** Emitted, when the status of a message is changed */
  void msgChanged(KMFolder*, Q_UINT32 sernum, int delta);

  /** Emitted when a field of the header of a specific message changed. */
  void msgHeaderChanged(KMFolder*, int);

  /** Emmited to display a message somewhere in a status line. */
  void statusMsg(const QString&);

  /** Emitted when number of unread messages has changed. */
  void numUnreadMsgsChanged( KMFolder* );

  // Emitted by KMFolderCachedIMAP to signal syncing
  void syncRunning( KMFolder*, bool );

public slots:
  /** Incrementally update the index if possible else call writeIndex */
  virtual int updateIndex() = 0;

  /** Add the message to the folder after it has been retrieved from an IMAP
      server */
  virtual void reallyAddMsg(KMMessage* aMsg);

  /** Add a copy of the message to the folder after it has been retrieved
      from an IMAP server */
  virtual void reallyAddCopyOfMsg(KMMessage* aMsg);

protected slots:
  virtual void removeJob( QObject* );

protected:
  virtual void addJob( FolderJob* ) const;
  /**
   * These two methods actually create the jobs. They have to be implemented
   * in all folders.
   * @see createJob
   */
  virtual FolderJob* doCreateJob( KMMessage *msg, FolderJob::JobType jt, KMFolder *folder,
                                  QString partSpecifier, const AttachmentStrategy *as ) const = 0;
  virtual FolderJob* doCreateJob( QPtrList<KMMessage>& msgList, const QString& sets,
                                  FolderJob::JobType jt, KMFolder *folder ) const = 0;

  /** Tell the folder that a header field that is usually used for
    the index (subject, from, ...) has changed of given message.
    This method is usually called from within KMMessage::setSubject/set... */
  void headerOfMsgChanged(const KMMsgBase*, int idx);

  /** Load message from file and store it at given index. Returns 0
    on failure. */
  virtual KMMessage* readMsg(int idx) = 0;

  /** Read index file and fill the message-info list mMsgList. */
  virtual bool readIndex() = 0;

  /** Called by KMFolder::remove() to delete the actual contents.
    At the time of the call the folder has already been closed, and
    the various index files deleted.  Returns 0 on success. */
  virtual int removeContents() = 0;

  /** Called by KMFolder::expunge() to delete the actual contents.
    At the time of the call the folder has already been closed, and
    the various index files deleted.  Returns 0 on success. */
  virtual int expungeContents() = 0;

  /** Read the config file */
  virtual void readConfig();

  virtual KMMsgBase* takeIndexEntry( int idx ) = 0;
  virtual KMMsgInfo* setIndexEntry( int idx, KMMessage *msg ) = 0;
  virtual void clearIndex(bool autoDelete=true, bool syncDict = false) = 0;
  virtual void fillDictFromIndex(KMMsgDict *dict) = 0;
  virtual void truncateIndex() = 0;

  int mOpenCount;
  int mQuiet;
  bool mChanged;
  /** is the automatic creation of a index file allowed ? */
  bool mAutoCreateIndex;
  /** if the index is dirty it will be recreated upon close() */
  bool mDirty;
  /** TRUE if the files of the folder are locked (writable) */
  bool mFilesLocked;
  /** nationalized label or QString::null (then name() should be used) */
  QString mLabel;
  /** name of the field that is used for "From" in listbox */
  QString mWhoField, mUserWhoField;
  KMAcctList* mAcctList;

  bool    mMailingListEnabled;
  QString mMailingListPostingAddress;
  QString mMailingListAdminAddress;
  uint    mIdentity;

  /** number of unread messages, -1 if not yet set */
  int mUnreadMsgs, mGuessedUnreadMsgs;
  int mTotalMsgs;
  bool mWriteConfigEnabled;
  /** sven: true if on destruct folder needs to be compacted. */
  bool needsCompact;
  /** false if index file is out of sync with mbox file */
  bool mCompactable;
  bool mNoContent;
  bool mConvertToUtf8;

  /** Points at the reverse dictionary for this folder. */
  KMMsgDictREntry *mRDict;
  /** List of jobs created by this folder.
   *  REMEBER to add jobs created via createJob
   *  to this list.
   */
  mutable QPtrList<FolderJob> mJobList;

  QTimer *mDirtyTimer;
  enum { mDirtyTimerInterval = 600000 }; // 10 minutes

  KMFolder* mFolder;
};

#endif // FOLDERSTORAGE_H

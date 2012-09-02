/*
  Copyright (c) 2012 Montel Laurent <montel@kde.org>

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

/* based on filter_sylpheed filter */

#include "filter_clawsmail.h"

#include <klocale.h>
#include <kfiledialog.h>
#include <kdebug.h>
#include <QDomDocument>
#include <QDomElement>

using namespace MailImporter;

/** Default constructor. */
FilterClawsMail::FilterClawsMail() :
  Filter( i18n( "Import Claws-mail Maildirs and Folder Structure" ),
          "Laurent Montel",
          i18n( "<p><b>Claws-mail import filter</b></p>"
                "<p>Select the base directory of the Claws-mail mailfolder you want to import "
                "(usually: ~/Mail ).</p>"
                "<p>Since it is possible to recreate the folder structure, the folders "
                "will be stored under: \"ClawsMail-Import\" in your local folder.</p>"
                "<p>This filter also recreates the status of message, e.g. new or forwarded.</p>") )
{
}

/** Destructor. */
FilterClawsMail::~FilterClawsMail()
{
}

QString FilterClawsMail::defaultSettingsPath()
{
  return QDir::homePath() + QLatin1String( "/.claws-mail/" );
}

QString FilterClawsMail::localMailDirPath()
{
  QFile folderListFile( FilterClawsMail::defaultSettingsPath() + QLatin1String( "/folderlist.xml" ) );
  if ( folderListFile.exists() ) {
    QDomDocument doc;
    QString errorMsg;
    int errorRow;
    int errorCol;
    if ( !doc.setContent( &folderListFile, &errorMsg, &errorRow, &errorCol ) ) {
      kDebug() << "Unable to load document.Parse error in line " << errorRow
               << ", col " << errorCol << ": " << errorMsg;
      return QString();
    }
    QDomElement settings = doc.documentElement();

    if ( settings.isNull() ) {
      return QString();
    }

    for ( QDomElement e = settings.firstChildElement(); !e.isNull(); e = e.nextSiblingElement() ) {
      if ( e.tagName() == QLatin1String( "folder" ) ) {
        if ( e.hasAttribute( "type" ) ) {
          if ( e.attribute( "type" ) == QLatin1String( "mh" ) ) {
            return QDir::homePath() + QDir::separator() + e.attribute("path" );
          }   
        }
      }
    }
  }
  return QString();
}

/** Recursive import of Sylpheed maildir. */
void FilterClawsMail::import()
{
  QString homeDir = localMailDirPath();
  if ( homeDir.isEmpty() )
    homeDir = QDir::homePath();

  KFileDialog *kfd = new KFileDialog( homeDir, "", 0 );
  kfd->setMode( KFile::Directory | KFile::LocalOnly );
  kfd->exec();
  const QString maildir = kfd->selectedFile();
  delete kfd;
  importMails( maildir );
}

void FilterClawsMail::processDirectory( const QString& path)
{
  QDir dir(path);
  const QStringList rootSubDirs = dir.entryList(QStringList("[^\\.]*"), QDir::Dirs , QDir::Name);
  QStringList::ConstIterator end = rootSubDirs.constEnd();
  for(QStringList::ConstIterator filename = rootSubDirs.constBegin() ; filename != end ; ++filename ) {
    if(filterInfo()->shouldTerminate())
      break;
    importDirContents(dir.filePath(*filename));
    filterInfo()->setOverall((int) ((float) mImportDirDone / mTotalDir * 100));
    mImportDirDone++;
  }
}

void FilterClawsMail::importMails( const QString &maildir )
{
  setMailDir(maildir);
  if ( mailDir().isEmpty() ) {
    filterInfo()->alert( i18n( "No directory selected." ) );
    return;
  }
  /**
   * If the user only select homedir no import needed because
   * there should be no files and we surely import wrong files.
   */
  else if ( mailDir() == QDir::homePath() || mailDir() == ( QDir::homePath() + '/' ) ) {
    filterInfo()->addErrorLogEntry( i18n( "No files found for import." ) );
  } else {
    filterInfo()->setOverall(0);

    mImportDirDone = 0;
    
    /** Recursive import of the MailFolders */
    QDir dir(mailDir());

    mTotalDir = Filter::countDirectory( dir, false );
    processDirectory( mailDir() );
    
    filterInfo()->addInfoLogEntry( i18n("Finished importing emails from %1", mailDir() ));
    if (countDuplicates() > 0) {
      filterInfo()->addInfoLogEntry( i18np("1 duplicate message not imported", "%1 duplicate messages not imported", countDuplicates()));
    }
  }
  if (filterInfo()->shouldTerminate())
    filterInfo()->addInfoLogEntry( i18n("Finished import, canceled by user."));
  setCountDuplicates(0);
  filterInfo()->setCurrent(100);
  filterInfo()->setOverall(100);
}

/**
 * Import of a directory contents.
 * @param info Information storage for the operation.
 * @param dirName The name of the directory to import.
 */
void FilterClawsMail::importDirContents( const QString& dirName)
{
  if(filterInfo()->shouldTerminate()) return;

  /** Here Import all archives in the current dir */
  importFiles(dirName);

  /** If there are subfolders, we import them one by one */
  processDirectory( dirName );
}

bool FilterClawsMail::excludeFile(const QString& file)
{
    if(file.endsWith(QLatin1String(".claws_cache")) ||
       file.endsWith(QLatin1String(".claws_mark")) ||
       file.endsWith(QLatin1String(".mh_sequences")) ) {
        return true;
    }
    return false;
}


/**
 * Import the files within a Folder.
 * @param info Information storage for the operation.
 * @param dirName The name of the directory to import.
 */
void FilterClawsMail::importFiles( const QString& dirName)
{
  QDir dir(dirName);
  QString _path;
  bool generatedPath = false;

  QHash<QString,unsigned long> msgflags;

  QDir importDir (dirName);
  const QStringList files = importDir.entryList(QStringList("[^\\.]*"), QDir::Files, QDir::Name);
  int currentFile = 1, numFiles = files.size();

  readMarkFile(dir.filePath(".claws_mark"), msgflags);

  QStringList::ConstIterator end( files.constEnd() );
  for ( QStringList::ConstIterator mailFile = files.constBegin(); mailFile != end; ++mailFile, ++currentFile) {
    if(filterInfo()->shouldTerminate())
      return;
    QString _mfile = *mailFile;
    if (!excludeFile(_mfile)) {
      if(!generatedPath) {
        //FIXME: Why recreate all the time _path ?

        _path = i18nc("define folder name where we will import clawsmail mails", "ClawsMail-Import") + QLatin1Char('/');
        QString _tmp = dir.filePath(*mailFile);
        _tmp = _tmp.remove(_tmp.length() - _mfile.length() -1, _mfile.length()+1);
        _path += _tmp.remove( mailDir(), Qt::CaseSensitive );
        QString _info = _path;
        filterInfo()->addInfoLogEntry(i18n("Import folder %1...", _info.remove(0,15)));

        filterInfo()->setFrom(_info);
        filterInfo()->setTo(_path);
        generatedPath = true;
      }

      Akonadi::MessageStatus status;
      if (msgflags[_mfile])
        status = msgFlagsToString((msgflags[_mfile]));
      else
        status.setRead( true ); // 0 == read
      if(filterInfo()->removeDupMessage()) {
        if(! addMessage( _path, dir.filePath(*mailFile),status )) {
          filterInfo()->addErrorLogEntry( i18n("Could not import %1", *mailFile ) );
        }
        filterInfo()->setCurrent((int) ((float) currentFile / numFiles * 100));
      } else {
        if(! addMessage_fastImport( _path, dir.filePath(*mailFile),status )) {
          filterInfo()->addErrorLogEntry( i18n("Could not import %1", *mailFile ) );
        }
        filterInfo()->setCurrent((int) ((float) currentFile / numFiles * 100));
      }
    }
  }
}


void FilterClawsMail::readMarkFile( const QString &path, QHash<QString,unsigned long> &dict )
{
  /* Each sylpheed mail directory contains a .sylpheed_mark file which
   * contains all the flags for each messages. The layout of this file
   * is documented in the source code of sylpheed: in procmsg.h for
   * the flag bits, and procmsg.c.
   *
   * Note that the mark file stores 32 bit unsigned integers in the
   * platform's native "endianness".
   *
   * The mark file starts with a 32 bit unsigned integer with a version
   * number. It is then followed by pairs of 32 bit unsigned integers,
   * the first one with the message file name (which is a number),
   * and the second one with the actual message flags */

  quint32 in, flags;
  QFile file(path);

  if (!file.open(QIODevice::ReadOnly))
    return;

  QDataStream stream(&file);

  if (Q_BYTE_ORDER == Q_LITTLE_ENDIAN)
    stream.setByteOrder(QDataStream::LittleEndian);



  /* Read version; if the value is reasonably too big, we're looking
   * at a file created on another platform. I don't have any test
   * marks/folders, so just ignoring this case */
  stream >> in;
  if (in > (quint32) 0xffff)
    return;

  while (!stream.atEnd()) {
    if(filterInfo()->shouldTerminate()){
      file.close();
      return;
    }
    stream >> in;
    stream >> flags;
    QString s;
    s.setNum((uint) in);
    dict.insert(s, flags);
  }
}

Akonadi::MessageStatus FilterClawsMail::msgFlagsToString(unsigned long flags)
{
  Akonadi::MessageStatus status;
  /* see sylpheed's procmsg.h */
  if (flags & 2UL)
    status.setRead( false );
  if ((flags & 3UL) == 0UL)
    status.setRead( true );
  if (flags & 8UL)
    status.setDeleted( true );
  if (flags & 16UL)
    status.setReplied( true );
  if (flags & 32UL)
    status.setForwarded( true );
  return status;
}

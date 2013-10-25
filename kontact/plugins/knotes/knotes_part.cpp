/*
  This file is part of the KDE project

  Copyright (C) 2002-2003 Daniel Molkentin <molkentin@kde.org>
  Copyright (C) 2004-2006 Michael Brade <brade@kde.org>
  Copyright (C) 2013 Laurent Montel <montel@kde.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "knotes_part.h"
#include "knoteseditdialog.h"
#include "knotesadaptor.h"
#include "knotesiconview.h"
#include "knoteswidget.h"
#include "knotetip.h"
#include "knotes/configdialog/knoteconfigdialog.h"
#include "knotes/network/knotesnetrecv.h"
#include "knotes/print/knoteprinter.h"
#include "knotes/print/knoteprintobject.h"
#include "knotes/print/knoteprintselectthemedialog.h"
#include "knotes/resource/resourcemanager.h"
#include "knotes/knoteedit.h"
#include "knotes/knotesglobalconfig.h"
#include "knotes/configdialog/knotesimpleconfigdialog.h"
#include "knoteutils.h"
#include "alarms/knotealarmdialog.h"
#include "alarms/knotesalarm.h"
#include <KCal/Journal>
using namespace KCal;

#include <KActionCollection>
#include <KAction>
#include <KInputDialog>
#include <KMessageBox>
#include <KXMLGUIFactory>
#include <KPrintPreview>
#include <ksocketfactory.h>
#include <KApplication>
#include <KFileDialog>

#include <QApplication>
#include <QClipboard>
#include <QTcpServer>
#include <QMenu>

#include <dnssd/publicservice.h>

KNotesPart::KNotesPart( KNotesResourceManager *manager, QObject *parent )
    : KParts::ReadOnlyPart( parent ),
      mNotesWidget( new KNotesWidget(this) ),
      mNoteTip( new KNoteTip( mNotesWidget->notesView() ) ),
      mManager( manager ),
      mListener(0),
      mPublisher(0),
      mAlarm(0),
      mNotePrintPreview(0)
{
    (void) new KNotesAdaptor( this );
    QDBusConnection::sessionBus().registerObject( QLatin1String("/KNotes"), this );

    setComponentData( KComponentData( "knotes" ) );

    // create the actions
    mNewNote = new KAction( KIcon( QLatin1String("knotes") ),
                            i18nc( "@action:inmenu create new popup note", "&New" ), this );
    actionCollection()->addAction( QLatin1String("file_new"), mNewNote );
    connect( mNewNote, SIGNAL(triggered(bool)), SLOT(newNote()) );
    mNewNote->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_N ) );
    mNewNote->setHelpText(
                i18nc( "@info:status", "Create a new popup note" ) );
    mNewNote->setWhatsThis(
                i18nc( "@info:whatsthis",
                       "You will be presented with a dialog where you can add a new popup note." ) );

    mNoteEdit = new KAction( KIcon( QLatin1String("document-edit") ),
                          i18nc( "@action:inmenu", "Edit..." ), this );
    actionCollection()->addAction( QLatin1String("edit_note"), mNoteEdit );
    connect( mNoteEdit, SIGNAL(triggered(bool)), SLOT(editNote()) );
    mNoteEdit->setHelpText(
                i18nc( "@info:status", "Edit popup note" ) );
    mNoteEdit->setWhatsThis(
                i18nc( "@info:whatsthis",
                       "You will be presented with a dialog where you can modify an existing popup note." ) );

    mNoteRename = new KAction( KIcon( QLatin1String("edit-rename") ),
                          i18nc( "@action:inmenu", "Rename..." ), this );
    actionCollection()->addAction( QLatin1String("edit_rename"), mNoteRename );
    connect( mNoteRename, SIGNAL(triggered(bool)), SLOT(renameNote()) );
    mNoteRename->setHelpText(
                i18nc( "@info:status", "Rename popup note" ) );
    mNoteRename->setWhatsThis(
                i18nc( "@info:whatsthis",
                       "You will be presented with a dialog where you can rename an existing popup note." ) );

    mNoteDelete = new KAction( KIcon( QLatin1String("edit-delete") ),
                          i18nc( "@action:inmenu", "Delete" ), this );
    actionCollection()->addAction( QLatin1String("edit_delete"), mNoteDelete );
    connect( mNoteDelete, SIGNAL(triggered(bool)), SLOT(killSelectedNotes()) );
    mNoteDelete->setShortcut( QKeySequence( Qt::Key_Delete ) );
    mNoteDelete->setHelpText(
                i18nc( "@info:status", "Delete popup note" ) );
    mNoteDelete->setWhatsThis(
                i18nc( "@info:whatsthis",
                       "You will be prompted if you really want to permanently remove "
                       "the selected popup note." ) );

    mNotePrint = new KAction( KIcon( QLatin1String("document-print") ),
                          i18nc( "@action:inmenu", "Print Selected Notes..." ), this );
    actionCollection()->addAction( QLatin1String("print_note"), mNotePrint );
    connect( mNotePrint, SIGNAL(triggered(bool)), SLOT(slotPrintSelectedNotes()) );
    mNotePrint->setHelpText(
                i18nc( "@info:status", "Print popup note" ) );
    mNotePrint->setWhatsThis(
                i18nc( "@info:whatsthis",
                       "You will be prompted to print the selected popup note." ) );

    if(KPrintPreview::isAvailable()) {

        mNotePrintPreview = new KAction( KIcon( QLatin1String("document-print-preview") ),i18nc( "@action:inmenu", "Print Preview Selected Notes..." ), this );
        actionCollection()->addAction( QLatin1String("print_preview_note"), mNotePrintPreview );

        connect( mNotePrintPreview, SIGNAL(triggered(bool)), SLOT(slotPrintPreviewSelectedNotes()) );
    }

    mNoteConfigure  = new KAction( KIcon( QLatin1String("configure") ), i18n( "Note settings..." ), this );
    actionCollection()->addAction( QLatin1String("configure_note"), mNoteConfigure );
    connect( mNoteConfigure, SIGNAL(triggered(bool)), SLOT(slotNotePreferences()) );

    KAction *act  = new KAction( KIcon( QLatin1String("configure") ), i18n( "Preferences KNotes..." ), this );
    actionCollection()->addAction( QLatin1String("knotes_configure"), act );
    connect( act, SIGNAL(triggered(bool)), SLOT(slotPreferences()) );

    mNoteSendMail = new KAction( KIcon( QLatin1String("mail-send") ), i18n( "Mail..." ), this );
    actionCollection()->addAction( QLatin1String("mail_note"), mNoteSendMail );
    connect( mNoteSendMail, SIGNAL(triggered(bool)), SLOT(slotMail()) );

    mNoteSendNetwork  = new KAction( KIcon( QLatin1String("network-wired") ), i18n( "Send..." ), this );
    actionCollection()->addAction( QLatin1String("send_note"), mNoteSendNetwork );
    connect( mNoteSendNetwork, SIGNAL(triggered(bool)), SLOT(slotSendToNetwork()) );

    mNoteSetAlarm  = new KAction( KIcon( QLatin1String("knotes_alarm") ), i18n( "Set Alarm..." ), this );
    actionCollection()->addAction( QLatin1String("set_alarm"), mNoteSetAlarm );
    connect( mNoteSetAlarm, SIGNAL(triggered(bool)), SLOT(slotSetAlarm()) );

    act  = new KAction( KIcon( QLatin1String("edit-paste") ),
                           i18n( "New Note From Clipboard" ), this );
    actionCollection()->addAction( QLatin1String("new_note_clipboard"), act );
    connect( act, SIGNAL(triggered()), SLOT(slotNewNoteFromClipboard()) );


    mSaveAs  = new KAction( KIcon( QLatin1String("document-save-as") ), i18n( "Save As..." ), this );
    actionCollection()->addAction( QLatin1String("save_note"), mSaveAs );
    connect( mSaveAs, SIGNAL(triggered(bool)), SLOT(slotSaveAs()) );

    // TODO icons: s/editdelete/knotes_delete/ or the other way round in knotes

    // set the view up

    connect( mNotesWidget->notesView(), SIGNAL(executed(QListWidgetItem*)),
             this, SLOT(editNote(QListWidgetItem*)) );

    connect( mNotesWidget->notesView(), SIGNAL(entered(QModelIndex)),
             this, SLOT(requestToolTip(QModelIndex)));

    connect( mNotesWidget->notesView(), SIGNAL(viewportEntered()),
             this, SLOT(hideToolTip()));

    connect( mNotesWidget->notesView(), SIGNAL(itemSelectionChanged()),
             this, SLOT(slotOnCurrentChanged()) );

    slotOnCurrentChanged();

    setWidget( mNotesWidget );
    setXMLFile( QLatin1String("knotes_part.rc") );

    // connect the resource manager
    connect( mManager, SIGNAL(sigRegisteredNote(KCal::Journal*)),
             this, SLOT(createNote(KCal::Journal*)) );
    connect( mManager, SIGNAL(sigDeregisteredNote(KCal::Journal*)),
             this, SLOT(killNote(KCal::Journal*)) );
    mManager->load();
    mAlarm = new KNotesAlarm( mManager, this );
    updateNetworkListener();
}

KNotesPart::~KNotesPart()
{
    delete mListener;
    mListener=0;
    delete mPublisher;
    mPublisher=0;
    delete mNoteTip;
    mNoteTip = 0;
}

void KNotesPart::requestToolTip( const QModelIndex &index )
{
    QRect m_itemRect = mNotesWidget->notesView()->visualRect( index );
    mNoteTip->setNote(
                static_cast<KNotesIconViewItem *>( mNotesWidget->notesView()->itemAt( m_itemRect.topLeft() ) ) );
}

void KNotesPart::hideToolTip()
{
    mNoteTip->setNote( 0 );
}

void KNotesPart::slotPrintPreviewSelectedNotes()
{
    printSelectedNotes(true);
}

void KNotesPart::slotPrintSelectedNotes()
{
    printSelectedNotes(false);
}

void KNotesPart::printSelectedNotes(bool preview)
{
    QList<QListWidgetItem *> lst = mNotesWidget->notesView()->selectedItems();
    if ( lst.isEmpty() ) {
        KMessageBox::information(
                    mNotesWidget,
                    i18nc( "@info",
                           "To print notes, first select the notes to print from the list." ),
                    i18nc( "@title:window", "Print Popup Notes" ) );
        return;
    }

    KNotesGlobalConfig *globalConfig = KNotesGlobalConfig::self();
    QString printingTheme = globalConfig->theme();
    if (printingTheme.isEmpty()) {
        QPointer<KNotePrintSelectThemeDialog> dlg = new KNotePrintSelectThemeDialog(widget());
        if (dlg->exec()) {
            printingTheme = dlg->selectedTheme();
        }
        delete dlg;
    }
    if (!printingTheme.isEmpty()) {

        QList<KNotePrintObject *> listPrintObj;
        foreach ( QListWidgetItem *item, lst ) {
            listPrintObj.append(new KNotePrintObject(static_cast<KNotesIconViewItem *>( item )->journal()));
        }
        KNotePrinter printer;
        printer.printNotes( listPrintObj, printingTheme, preview );
        qDeleteAll(listPrintObj);
    }
}

bool KNotesPart::openFile()
{
    return false;
}

// public KNotes D-Bus interface implementation

QString KNotesPart::newNote( const QString &name, const QString &text )
{
    // create the new note
    Journal *journal = new Journal();

    // new notes have the current date/time as title if none was given
    if ( !name.isEmpty() ) {
        journal->setSummary( name );
    } else {
        journal->setSummary( KGlobal::locale()->formatDateTime( QDateTime::currentDateTime() ) );
    }

    // the body of the note
    journal->setDescription( text );

    // Edit the new note if text is empty
    if ( text.isNull() ) {
        QPointer<KNoteEditDialog> dlg = new KNoteEditDialog( widget() );

        dlg->setTitle( journal->summary() );
        dlg->setText( journal->description() );


        const QString property = journal->customProperty("KNotes", "RichText");
        if ( !property.isNull() ) {
            dlg->setAcceptRichText( property == QLatin1String("true") ? true : false );
        } else {
            KNotesGlobalConfig *globalConfig = KNotesGlobalConfig::self();
            dlg->setAcceptRichText( globalConfig->richText());
        }


        dlg->noteEdit()->setFocus();
        if ( dlg->exec() == QDialog::Accepted ) {
            journal->setSummary( dlg->title() );
            journal->setDescription( dlg->text() );
            delete dlg;
        } else {
            delete dlg;
            delete journal;
            return QString();
        }
    }

    mManager->addNewNote( journal );

    KNotesIconViewItem *note = mNoteList.value( journal->uid() );
    mNotesWidget->notesView()->scrollToItem( note );
    mNotesWidget->notesView()->setCurrentItem( note );
    mManager->save();
    return journal->uid();
}

QString KNotesPart::newNoteFromClipboard( const QString &name )
{
    const QString &text = QApplication::clipboard()->text();
    return newNote( name, text );
}

void KNotesPart::killNote( const QString &id )
{
    killNote( id, false );
}

void KNotesPart::killNote( const QString &id, bool force )
{
    KNotesIconViewItem *note = mNoteList.value( id );

    if ( note &&
         ( (!force && KMessageBox::warningContinueCancelList(
                mNotesWidget,
                i18nc( "@info", "Do you really want to delete this note?" ),
                QStringList( mNoteList.value( id )->text() ),
                i18nc( "@title:window", "Confirm Delete" ),
                KStandardGuiItem::del() ) == KMessageBox::Continue )
           || force ) ) {
        KNoteUtils::removeNote(mNoteList.value( id )->journal(), 0);
        mManager->deleteNote( mNoteList.value( id )->journal() );
        mManager->save();
    }
}

QString KNotesPart::name( const QString &id ) const
{
    KNotesIconViewItem *note = mNoteList.value( id );
    if ( note ) {
        return note->text();
    } else {
        return QString();
    }
}

QString KNotesPart::text( const QString &id ) const
{
    KNotesIconViewItem *note = mNoteList.value( id );
    if ( note ) {
        return note->journal()->description();
    } else {
        return QString();
    }
}

void KNotesPart::setName( const QString &id, const QString &newName )
{
    KNotesIconViewItem *note = mNoteList.value( id );
    if ( note ) {
        note->setIconText( newName );
        mManager->save();
    }
}

void KNotesPart::setText( const QString &id, const QString &newText )
{
    KNotesIconViewItem *note = mNoteList.value( id );
    if ( note ) {
        note->journal()->setDescription( newText );
        mManager->save();
    }
}

QMap<QString, QString> KNotesPart::notes() const
{
    QMap<QString, QString> notes;

    QHashIterator<QString, KNotesIconViewItem*> i(mNoteList);
    while ( i.hasNext() ) {
        i.next();
        notes.insert( i.value()->journal()->uid(), i.value()->journal()->summary() );
    }
    return notes;
}

// private stuff

void KNotesPart::killSelectedNotes()
{
    QList<QListWidgetItem *> lst = mNotesWidget->notesView()->selectedItems ();
    if ( lst.isEmpty() ) {
        return;
    }
    QStringList notes;
    QList<KNotesIconViewItem*> items;

    foreach ( QListWidgetItem *item, lst ) {
        KNotesIconViewItem *knivi = static_cast<KNotesIconViewItem *>( item );
        items.append( knivi );
        notes.append( knivi->realName() );
    }

    int ret = KMessageBox::warningContinueCancelList(
                mNotesWidget,
                i18ncp( "@info",
                        "Do you really want to delete this note?",
                        "Do you really want to delete these %1 notes?", items.count() ),
                notes, i18nc( "@title:window", "Confirm Delete" ),
                KStandardGuiItem::del() );

    if ( ret == KMessageBox::Continue ) {
        QListIterator<KNotesIconViewItem*> kniviIt( items );
        while ( kniviIt.hasNext() ) {
            Journal *journal = kniviIt.next()->journal();
            KNoteUtils::removeNote(journal, 0);
            mManager->deleteNote( journal );
        }
        mManager->save();
    }
}

void KNotesPart::popupRMB( QListWidgetItem *item, const QPoint &pos, const QPoint &globalPos )
{
    Q_UNUSED( item );

    QMenu *contextMenu = new QMenu(widget());
    if ( mNotesWidget->notesView()->itemAt ( pos ) ) {
        contextMenu->addAction(mNewNote);
        const bool uniqueNoteSelected = (mNotesWidget->notesView()->selectedItems().count() == 1);
        if (uniqueNoteSelected) {
            contextMenu->addSeparator();
            contextMenu->addAction(mSaveAs);
            contextMenu->addSeparator();
            contextMenu->addAction(mNoteEdit);
            contextMenu->addAction(mNoteRename);
            contextMenu->addSeparator();
            contextMenu->addAction(mNoteSendMail);
            contextMenu->addSeparator();
            contextMenu->addAction(mNoteSendNetwork);
        }
        contextMenu->addSeparator();
        contextMenu->addAction(mNotePrint);

        if (uniqueNoteSelected) {
            contextMenu->addSeparator();
            contextMenu->addAction(mNoteConfigure);
        }
        contextMenu->addSeparator();
        contextMenu->addAction(mNoteDelete);
    } else {
        contextMenu->addAction(mNewNote);
    }

    contextMenu->exec( mNotesWidget->notesView()->mapFromParent( globalPos ) );
    delete contextMenu;
}

void KNotesPart::mouseMoveOnListWidget( const QPoint & pos )
{
    QListWidgetItem *item = mNotesWidget->notesView()->itemAt( pos );
    mNoteTip->setNote( dynamic_cast<KNotesIconViewItem *>( item ) );
}

// TODO: also with takeItem, clear(),

// create and kill the icon view item corresponding to the note, edit the note

void KNotesPart::createNote( KCal::Journal *journal )
{
    mNoteList.insert( journal->uid(), new KNotesIconViewItem( mNotesWidget->notesView(), journal ) );
}

void KNotesPart::killNote( KCal::Journal *journal )
{
    KNotesIconViewItem *item = mNoteList.take( journal->uid() );
    delete item;
}

void KNotesPart::editNote( QListWidgetItem *item )
{
    QPointer<KNoteEditDialog> dlg = new KNoteEditDialog( widget() );
    KNotesIconViewItem * knotesItem = static_cast<KNotesIconViewItem *>( item );

    Journal *journal = knotesItem->journal();
    dlg->setTitle( journal->summary() );
    dlg->setText( journal->description() );
    dlg->setReadOnly(knotesItem->readOnly());

    const QString property = journal->customProperty("KNotes", "RichText");
    if ( !property.isNull() ) {
        dlg->setAcceptRichText( property == QLatin1String("true") ? true : false );
    } else {
        KNotesGlobalConfig *globalConfig = KNotesGlobalConfig::self();
        dlg->setAcceptRichText( globalConfig->richText());
    }

    dlg->noteEdit()->setFocus();
    if ( dlg->exec() == QDialog::Accepted ) {
        static_cast<KNotesIconViewItem *>( item )->setIconText( dlg->title() );
        journal->setDescription( dlg->text() );
        mManager->save();
    }
    delete dlg;
}

void KNotesPart::editNote()
{
    QListWidgetItem *item = mNotesWidget->notesView()->currentItem();
    if ( item ) {
        editNote( item );
    }
}

void KNotesPart::renameNote()
{
    KNotesIconViewItem *knoteItem = static_cast<KNotesIconViewItem *>(mNotesWidget->notesView()->currentItem());

    QString oldName = knoteItem->realName();
    bool ok = false;
    QString newName =
            KInputDialog::getText( i18nc( "@title:window", "Rename Popup Note" ),
                                   i18nc( "@label:textbox", "New Name:" ),
                                   oldName, &ok, mNotesWidget );
    if ( ok && ( newName != oldName ) ) {
        knoteItem->setIconText( newName );
        mManager->save();
    }
}

void KNotesPart::slotOnCurrentChanged( )
{
    const bool uniqueNoteSelected = (mNotesWidget->notesView()->selectedItems().count() == 1);
    const bool enabled(mNotesWidget->notesView()->currentItem());
    mNoteRename->setEnabled( enabled && uniqueNoteSelected);
    mNoteEdit->setEnabled( enabled && uniqueNoteSelected);
    mNoteConfigure->setEnabled( uniqueNoteSelected );
    mNoteSendMail->setEnabled(uniqueNoteSelected);
    mNoteSendNetwork->setEnabled(uniqueNoteSelected);
    mNoteSetAlarm->setEnabled(uniqueNoteSelected);
    mSaveAs->setEnabled(uniqueNoteSelected);
}

void KNotesPart::slotNotePreferences()
{
    if (!mNotesWidget->notesView()->currentItem())
        return;

    KNotesIconViewItem *knoteItem = static_cast<KNotesIconViewItem *>(mNotesWidget->notesView()->currentItem());
    const QString name = knoteItem->realName();
    QPointer<KNoteSimpleConfigDialog> dialog = new KNoteSimpleConfigDialog( knoteItem->config(), name, widget(), knoteItem->journal()->uid() );
    connect( dialog, SIGNAL(settingsChanged(QString)) , this,
             SLOT(slotApplyConfig()) );
    dialog->exec();
    delete dialog;
}

void KNotesPart::slotApplyConfig()
{
    KNotesIconViewItem *knoteItem = static_cast<KNotesIconViewItem *>(mNotesWidget->notesView()->currentItem());
    knoteItem->updateColor();
    mManager->save();
}

void KNotesPart::slotPreferences()
{
    // create a new preferences dialog...
    KNoteConfigDialog *dialog = new KNoteConfigDialog( i18n( "Settings" ), widget());
    connect( dialog, SIGNAL(configWrote()), this, SLOT(slotConfigUpdated()));
    dialog->show();
}

void KNotesPart::slotConfigUpdated()
{
    updateNetworkListener();
}

void KNotesPart::slotMail()
{
    if (!mNotesWidget->notesView()->currentItem())
        return;
    KNotesIconViewItem *knoteItem = static_cast<KNotesIconViewItem *>(mNotesWidget->notesView()->currentItem());
    KNoteUtils::sendMail(widget(),knoteItem->realName(), knoteItem->journal()->description());
}

void KNotesPart::slotSendToNetwork()
{
    if (!mNotesWidget->notesView()->currentItem())
        return;
    KNotesIconViewItem *knoteItem = static_cast<KNotesIconViewItem *>(mNotesWidget->notesView()->currentItem());
    KNoteUtils::sendToNetwork(widget(),knoteItem->realName(), knoteItem->journal()->description());
}

void KNotesPart::updateNetworkListener()
{
    delete mListener;
    mListener=0;
    delete mPublisher;
    mPublisher=0;

    if ( KNotesGlobalConfig::receiveNotes() ) {
        // create the socket and start listening for connections
        mListener = KSocketFactory::listen( QLatin1String("knotes") , QHostAddress::Any,
                                           KNotesGlobalConfig::port() );
        connect( mListener, SIGNAL(newConnection()), SLOT(slotAcceptConnection()) );
        mPublisher=new DNSSD::PublicService(KNotesGlobalConfig::senderID(), QLatin1String("_knotes._tcp"), KNotesGlobalConfig::port());
        mPublisher->publishAsync();
    }
}

void KNotesPart::slotAcceptConnection()
{
    // Accept the connection and make KNotesNetworkReceiver do the job
    QTcpSocket *s = mListener->nextPendingConnection();

    if ( s ) {
        KNotesNetworkReceiver *recv = new KNotesNetworkReceiver( s );
        connect( recv,SIGNAL(sigNoteReceived(QString,QString)), SLOT(newNote(QString,QString)) );
    }
}

void KNotesPart::slotSetAlarm()
{
    if (!mNotesWidget->notesView()->currentItem())
        return;
    KNotesIconViewItem *knoteItem = static_cast<KNotesIconViewItem *>(mNotesWidget->notesView()->currentItem());

    QPointer<KNoteAlarmDialog> dlg = new KNoteAlarmDialog( knoteItem->realName(), widget() );
    dlg->setIncidence( knoteItem->journal() );
    if ( dlg->exec() ) {
        mManager->save();
    }
    delete dlg;
}

void KNotesPart::slotNewNoteFromClipboard()
{
    const QString &text = KApplication::clipboard()->text();
    newNote( QString(), text );
}

void KNotesPart::slotSaveAs()
{
    if (!mNotesWidget->notesView()->currentItem())
        return;
    KNotesIconViewItem *knoteItem = static_cast<KNotesIconViewItem *>(mNotesWidget->notesView()->currentItem());

    KUrl url;
    QPointer<KFileDialog> dlg = new KFileDialog( url, QString(), widget() );
    dlg->setOperationMode( KFileDialog::Saving );
    dlg->setCaption( i18n( "Save As" ) );
    if( !dlg->exec() ) {
        delete dlg;
        return;
    }

    const QString fileName = dlg->selectedFile();
    delete dlg;
    if ( fileName.isEmpty() ) {
        return;
    }

    QFile file( fileName );
    if ( file.exists() &&
         KMessageBox::warningContinueCancel( widget(),
                                             i18n( "<qt>A file named <b>%1</b> already exists.<br />"
                                                   "Are you sure you want to overwrite it?</qt>",
                                                   QFileInfo( file ).fileName() ) ) != KMessageBox::Continue ) {
        return;
    }

    if ( file.open( QIODevice::WriteOnly ) ) {
        QTextStream stream( &file );
        stream<<knoteItem->realName() + QLatin1Char('\n');
        //TODO verify richtext
        stream<<knoteItem->journal()->description();
    }
}

#include "knotes_part.moc"

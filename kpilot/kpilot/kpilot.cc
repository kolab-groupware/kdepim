// kpilot.cc
//
// Copyright (C) 1998,1999,2000 Dan Pilone
//
// This file is distributed under the Gnu General Public Licence (GPL).
// The GPL should have been included with this file in a file called
// COPYING.

// $Revision$
static const char *id="$Id$";


// REVISION HISTORY
//
// 3.1b9	By Dan Pilone
// 3.1b10	By Adriaan de Groot: comments added all over the place,
//		added debug-level variable. The error messages sent to
//		the user still need work. Fixed socket bugs thanks
//		to Robert Ambrose. Added dependency on pilotDaemon.h
//		to ensure compatibility.
//

#include "options.h"


#include <sys/types.h>
#include <dirent.h>
#include <getopt.h>
#include <iostream.h>
#include <fstream.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <qfile.h>
#include <qlist.h>
#include <qstring.h>
#include <qlistbox.h>
#include <qcombobox.h>

#include <kurl.h>
#include <kmessagebox.h>
#include <kstatusbar.h>
#include <kconfig.h>
#include <kwin.h>
#include <kprocess.h>
#include <ksock.h>
#include <kcombobox.h>
#include <kmenubar.h>
#include <kstddirs.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kiconloader.h>
#include <kdebug.h>

#include "kpilotOptions.h"
#include "kpilot.moc"
#include "kpilotlink.h"
#include "messageDialog.h"
#include "addressWidget.h"
#include "kpilot_on_pp.h"
#include "statusMessages.h"
#include "conduitSetup.h"
#include "pilotDaemon.h"


KPilotInstaller::KPilotInstaller()
  : KTMainWindow(), fMenuBar(0L), fStatusBar(0L), fToolBar(0L),
    fQuitAfterCopyComplete(false), fManagingWidget(0L), fPilotLink(0L),
    fPilotCommandSocket(0L), fPilotStatusSocket(0L), fKillDaemonOnExit(false)
{
	FUNCTIONSETUP;

	KConfig& config = KPilotLink::getConfig();
	// Log a warning if the config file is too old.
	//
	//
	(void) KPilotOptions::isNewer(config);

	readConfig(config);

    if(config.readNumEntry("NextUniqueID", 0) == 0)
      {
	// Is this an ok value to use??
	config.writeEntry("NextUniqueID", 0xf000);
	config.sync();
      }

	// Just a block to isolate the config file.
	//
	//
	{
	KConfig KDEGlobalConfig(QString::null);
	KDEGlobalConfig.setGroup("General");
	fixedFont = KDEGlobalConfig.readFontEntry("fixed");
	}

    initPilotLink();
	if (!fPilotCommandSocket)
	{
		kdError() << __FUNCTION__ << ": Couldn't connect to daemon -- quitting"
			<< endl;
		exit(1);
	}
    setupWidget();
    initComponents();
    initStatusLink();  // This is separate to allow components to initialize
    showTitlePage(QString::null,true);
    show();
    }

KPilotInstaller::~KPilotInstaller()
{
	FUNCTIONSETUP;

	if (fPilotStatusSocket) delete fPilotStatusSocket;
	fPilotStatusSocket=0L;

	if(fKillDaemonOnExit && fPilotCommandSocket &&
		(fPilotCommandSocket->socket()>=0))
	{
#ifdef DEBUG
		if (debug_level & SYNC_TEDIOUS)
		{
			kdDebug() << fname
				<< ": Killing daemon."
				<< endl;
		}
#endif

		ofstream out(fPilotCommandSocket->socket());
		out << -3 << endl;
	}

	if (fPilotCommandSocket) delete fPilotCommandSocket;
	fPilotCommandSocket=0L;
}

void KPilotInstaller::readConfig(KConfig& config)
{
	config.setGroup(QString::null);
	fKillDaemonOnExit = config.readBoolEntry("StopDaemonAtExit",false);
}


#include "kpilot.xpm"

void
KPilotInstaller::setupWidget()
    {
	FUNCTIONSETUP;
	QPixmap icon((const char **)kpilot);

	// FIXME: We need to load the mini icon
	KWin::setIcons(winId(), icon, icon);

    // KWM::setIcon(winId(), kapp->getIcon());
    setCaption("KPilot");
    setMinimumSize(500,405);
    // setMaximumSize(500,405);
    initIcons();
    initMenu();
    initStatusBar();
    initToolBar();
    fManagingWidget = new QWidget(this);
    fManagingWidget->setMinimumSize(500,330);
    fManagingWidget->show();
    setView(fManagingWidget, FALSE);
    }

void
KPilotInstaller::initComponents()
    {
	FUNCTIONSETUP;

    PilotComponent* aComponent;
    
    QLabel* titleScreen = new QLabel(getManagingWidget());
    titleScreen->setPixmap(QPixmap(kpilot_on_pp));
    titleScreen->setAlignment(AlignCenter);
    titleScreen->setBackgroundColor(QColor("black"));
    titleScreen->setGeometry(0, 0, getManagingWidget()->geometry().width(), 
			           getManagingWidget()->geometry().height());
    fVisibleWidgetList.append(titleScreen);
    
    aComponent = new MemoWidget(this, getManagingWidget());
    aComponent->initialize();
    fPilotComponentList.append(aComponent);
    aComponent = new AddressWidget(this, getManagingWidget());
    aComponent->initialize();
    fPilotComponentList.append(aComponent);
    aComponent = new FileInstallWidget(this, getManagingWidget());
    aComponent->initialize();
    fPilotComponentList.append(aComponent);
    }

void
KPilotInstaller::initStatusBar()
{
	FUNCTIONSETUP;
	QString welcomeMessage=i18n("Welcome to KPilot");
	welcomeMessage+=" (";
	welcomeMessage+=version(0);
	welcomeMessage+=')';

	fStatusBar = new KStatusBar(this, "statusbar");
	fStatusBar->insertItem(welcomeMessage,0);
	fStatusBar->show();
	setStatusBar(fStatusBar);
}

// These are XPM files disguised as .h files
// 
//
#include "hotsync.h"
#include "toolbar_backup.xpm"
#include "toolbar_restore.xpm"
#include "fastsync.xpm"

void KPilotInstaller::initIcons()
{
	FUNCTIONSETUP;

	DEBUGKPILOT << fname
		<< ": Getting Icon Loader"
		<< endl;

	KIconLoader *il = KGlobal::iconLoader();

	DEBUGKPILOT << fname
		<< ": Got Icon Loader @"
		<< (int) il
		<< endl;

	il->addAppDir("kpilot");

	DEBUGKPILOT << fname
		<< ": Added apps dir to Icon Loader"
		<< endl;

	icon_hotsync = il->loadIcon("hotsync",
		KIcon::Toolbar,0,KIcon::DefaultState,0, true);

	DEBUGKPILOT << fname
		<< ": Got first icon."
		<< endl;

	if (icon_hotsync.isNull())
	{
		kdWarning() << __FUNCTION__ << ": Hot-Sync icon not found." << endl;
		icon_hotsync=QPixmap((const char **)hotsync_icon);
	}

	icon_backup = il->loadIcon("backup",
		KIcon::Toolbar,0,KIcon::DefaultState,0, true);
	if (icon_backup.isNull())
	{
		kdWarning() << __FUNCTION__ << ": Backup icon not found." << endl;
		icon_backup =QPixmap((const char **)toolbar_backup);
	}

	icon_fastsync = il->loadIcon("fastsync",
		KIcon::Toolbar,0,KIcon::DefaultState,0, true);
	if (icon_fastsync.isNull())
	{
		kdWarning() << __FUNCTION__ << ": Fast-Sync icon not found." << endl;
		icon_fastsync = QPixmap((const char **)fastsync_xpm);
	}

	icon_restore = il->loadIcon("restore",
		KIcon::Toolbar,0,KIcon::DefaultState,0, true);
	if (icon_restore.isNull())
	{
		kdWarning() << __FUNCTION__ << ": Restore icon not found." << endl;
		icon_restore = QPixmap((const char **)toolbar_restore);
	}

	icon_quit = il->loadIcon("exit",
		KIcon::Toolbar,0,KIcon::DefaultState,0,true);

	if (icon_quit.isNull())
	{
		kdWarning() << __FUNCTION__ << ": Quit icon not found." << endl;
	}
}


void
KPilotInstaller::initToolBar()
  {
	FUNCTIONSETUP;

	fToolBar = new KToolBar(this, "toolbar");

	KConfig& c = KPilotLink::getConfig();
	QStringList s = c.readListEntry("ToolbarIcons");


	// We allow some kind of toolbar customisation,
	// and we should switch to the real KDE2 configurable
	// toolbars soon (after KDE 2.1 / KPilot 4.0.0)
	//
	//
	if (s.isEmpty() || s.contains("HotSync"))
	{
		fToolBar->insertButton(icon_hotsync, ID_FILE_HOTSYNC,
			SIGNAL(clicked(int)), this, SLOT(menuCallback(int)),
			TRUE, i18n("Hot-Sync"));
	}

	if (s.contains("FastSync"))
	{
		fToolBar->insertButton(icon_fastsync, ID_FILE_FASTSYNC,
			SIGNAL(clicked(int)),this,SLOT(menuCallback(int)),
			TRUE,i18n("Fast-Sync"));
	}

	if (s.isEmpty() || s.contains("Backup"))
	{
		// This next button exactly mirrors
		// the functionality of the menu back
		// "backup" choice.
		//
		//
		fToolBar->insertButton(icon_backup,ID_FILE_BACKUP,
			SIGNAL(clicked(int)),this,SLOT(menuCallback(int)),
			TRUE, i18n("Full Backup"));
	}

	if (s.contains("Restore"))
	{
		fToolBar->insertButton(icon_restore,ID_FILE_RESTORE,
			SIGNAL(clicked(int)),this,SLOT(menuCallback(int)),
			TRUE, i18n("Full Restore"));
	}




	// KDE2 style guide says "No quit toolbar button"
	//
	//
	// icon.load(kapp->kde_toolbardir() + "/exit.xpm");
	// 
	// fToolBar->insertButton( icon, 0, 
	//	SIGNAL(clicked()), this, SLOT(quit()),
	//	TRUE, i18n("Quit"));

	conduitCombo=new QComboBox(fToolBar,"conduitCombo");
	conduitCombo->insertItem(version(0));
	fToolBar->insertWidget(KPilotInstaller::ID_COMBO,140,conduitCombo,0);
	connect(conduitCombo,SIGNAL(activated(int)),
		this,SLOT(slotModeSelected(int)));

  fToolBar->alignItemRight(KPilotInstaller::ID_COMBO);
  addToolBar(fToolBar);
  }


void
KPilotInstaller::slotModeSelected(int selected)
{
	FUNCTIONSETUP;
	int current = 0 ;

#ifdef DEBUG
	if (debug_level& UI_TEDIOUS)
	{
		kdDebug() << fname << ": Responding to callback " << selected
			<< endl;
	}
#endif

	if((unsigned int)selected >= fVisibleWidgetList.count())
	{
		kdWarning() << __FUNCTION__ << ": Illegal component #" 
			<< selected << " selected.\n" << endl;
		return;
	}


	current=0;
	for (fVisibleWidgetList.first();
		fVisibleWidgetList.current();
		fVisibleWidgetList.next())
	{
		if (selected != current)
		{
			fVisibleWidgetList.current()->hide();
		}
		else
		{
			fVisibleWidgetList.at(selected)->show();
		}

		current++;
	}
		

	fStatusBar->changeItem(conduitCombo->text(selected),0);
}

void KPilotInstaller::showTitlePage(const QString& msg,bool force)
{
	FUNCTIONSETUP;

	if ((conduitCombo->currentItem()!=0) || force)
	{
		slotModeSelected(0);
		conduitCombo->setCurrentItem(0);
	}

	if (!msg.isNull())
	{
		fStatusBar->changeItem(msg,0);
	}
}

	



void
KPilotInstaller::initPilotLink()
{
	FUNCTIONSETUP;

	if(fPilotLink)
	{
		kdWarning() << __FUNCTION__ << ": Pilot Link already created?\n" ;
		return;
	}

	fPilotLink = new KPilotLink();
	if (fPilotLink==NULL)
	{
		kdError() << __FUNCTION__ << ": Can't allocate fPilotLink.\n";
		KMessageBox::error(this,
				   i18n("Allocating PilotLink failed."),
				   i18n("Cannot create link to Daemon"));
		return;
	}


	if(fPilotCommandSocket == NULL)
	{
		initCommandSocket();
	}
	else 
	{
		// We were called after a reconfigure
		//
		//
#ifdef DEBUG
		if (debug_level && SYNC_MAJOR)
		{
			kdDebug() << fname << ": Reconfiguring daemon.\n" ;
		}
#endif

		ofstream out(fPilotCommandSocket->socket());
		out << "-2" << endl;
	}
}

void KPilotInstaller::initCommandSocket()
{
	FUNCTIONSETUP;

	MessageDialog *messageDialog=0L;

	if (fPilotCommandSocket)
	{
		kdWarning() << __FUNCTION__ 
			<< ": We already have a command socket"
			<< endl;
		return;
	}

#ifdef DEBUG
	if (debug_level & SYNC_MINOR)
	{
		kdDebug() << fname
			<< ": Creating command socket"
			<< endl ;
	}
#endif

	fPilotCommandSocket = new KSocket("localhost", 
		PILOTDAEMON_COMMAND_PORT);
	if (fPilotCommandSocket==NULL)
	{
		kdError() << __FUNCTION__ << ": Can't allocate fPilotCommandSocket.\n";
		KMessageBox::error(this,
				   i18n("Allocating fPilotCommandSocket failed."),
				   i18n("Cannot create link to Daemon"));

		delete fPilotLink;
		fPilotLink=NULL;
		return;
	}

#ifdef DEBUG
	if (debug_level & SYNC_TEDIOUS)
	{
		kdDebug() << fname
			<< ": Got socket " 
			<< fPilotCommandSocket->socket()
			<< endl ;
	}
#endif

	
	if((fPilotCommandSocket->socket() < 0) ||
		!testSocket(fPilotCommandSocket))
	{
		int i;

#ifdef DEBUG
		if (debug_level & SYNC_MAJOR)
		{
			kdDebug() << fname
				<< ": Socket not OK, starting daemon"
				<< endl;
		}
#endif

		// It wasn't running...
		messageDialog = 
			new MessageDialog(version(0));

		if (messageDialog!=NULL)
		{
			messageDialog->show();
			kapp->processEvents();
			messageDialog->setMessage(
				i18n("Starting Sync Daemon. "
					"Please Wait."));

			kapp->processEvents();
		}

		delete fPilotCommandSocket;
		fPilotCommandSocket=0L;
	

		KProcess pilotDaemon;
		pilotDaemon << "kpilotDaemon";
#ifdef DEBUG
		if (debug_level)
		{
			QString s;
			s.setNum(debug_level);

			pilotDaemon << "--debug";
			pilotDaemon << s;
		}
#endif
			
		kapp->processEvents();

		pilotDaemon.start(KProcess::DontCare);

		// While the daemon is starting up,
		// we'll do some busy-waiting and
		// keep trying to connect to it.
		//
		//
		for (i=0; i<4; i++)
		{
			kapp->processEvents();
			sleep(1);
			kapp->processEvents();

#ifdef DEBUG
			if (debug_level & SYNC_TEDIOUS)
			{
				kdDebug() << fname
					<< ": Trying to connect"
					<< endl;
			}
#endif
			fPilotCommandSocket = new KSocket("localhost",
				PILOTDAEMON_COMMAND_PORT);
			if (!fPilotCommandSocket)
			{
				kdError() << __FUNCTION__
					<< ": Couldn't allocate KSocket"
					<< endl;
				continue;
			}

			if ((fPilotCommandSocket->socket() >= 0) &&
				testSocket(fPilotCommandSocket))
			{
				break;
			}
			else
			{
				delete fPilotCommandSocket;
				fPilotCommandSocket=0L;
			}
		}

#ifdef DEBUG
		if (debug_level & UI_MINOR)
		{
			kdDebug() << fname
				<< ": Halfway result "
				<< "PilotCommandSocket="
				<< (int)fPilotCommandSocket
				<< endl;
		}
#endif
			
		if(!fPilotCommandSocket)
		{
			kdError() << __FUNCTION__ 
				<< ": Can't connect to daemon"
				<< endl ;

			KMessageBox::error(this, 
					   i18n("Cannot start KPilot Daemon. "
						"Check settings and "
						"restart KPilot manually."),
					   i18n("Cannot connect to Daemon"));
					   

#ifdef DEBUG
			if (debug_level & UI_TEDIOUS)
			{
				kdDebug() << fname << ": creating options "
					<< endl;
			}
#endif
			KPilotOptions* options = new KPilotOptions(this);
			if (options)
			{
				options->exec();
#ifdef DEBUG
				if (debug_level & UI_TEDIOUS)
				{
					kdDebug() << fname << ": User said "
						<< options->result()
						<< endl;
				}
#endif
				delete options;
			}
			else
			{
				kdError() << __FUNCTION__
					<< ": Couldn't create options window."
					<< endl;
			}
		}

		fKillDaemonOnExit = true;

		if (messageDialog!=NULL)
		{
#ifdef DEBUG
			if (debug_level & UI_TEDIOUS)
			{
				kdDebug() << fname
					<< ": Closing and deleting msg dialog"
					<< endl;
			}
#endif
			messageDialog->close();
			delete messageDialog;
		}
	}

	fLinkCommand[0] = 0L;

#ifdef DEBUG
	if (debug_level & UI_MINOR)
	{
		kdDebug() << fname
			<< ": End result "
			<< "PilotCommandSocket="
			<< (int)fPilotCommandSocket
			<< endl;
	}
#endif
}


int KPilotInstaller::testSocket(KSocket *s)
{
	FUNCTIONSETUP;

	char buf[12];
	int r=0;
	int fd=0;

	if (!s) return 0;

	fd=s->socket();
	if (fd<0) return 0;

	sprintf(buf,"%d\n",KPilotLink::TestConnection);

	signal(SIGPIPE,SIG_IGN);
	r=write(fd,buf,strlen(buf));
	if (r<0)
	{
		int e=errno;
		kdError() << __FUNCTION__  
			<< ": (" << strerror(e) << ")" 
			<< endl;
	}
	signal(SIGPIPE,SIG_DFL);
	
	return (r>=0);
}


void
KPilotInstaller::initStatusLink()
{
	FUNCTIONSETUP;

	fPilotStatusSocket = new KSocket("localhost", 
		PILOTDAEMON_STATUS_PORT);
	if (fPilotStatusSocket->socket()!=-1)
	{
#ifdef DEBUG
		if (debug_level& SYNC_TEDIOUS)
		{
			kdDebug() << fname <<
				": Connected socket successfully.\n";
		}
#endif

		connect(fPilotStatusSocket, SIGNAL(readEvent(KSocket*)),
			this, SLOT(slotDaemonStatus(KSocket*)));
		fPilotStatusSocket->enableRead(true);
	}
	else
	{
		kdError() << __FUNCTION__ <<
			": Failed to connect socket to daemon.\n";
	}
}





void
KPilotInstaller::slotDaemonStatus(KSocket* daemon)
{
	FUNCTIONSETUP;

	ifstream in(daemon->socket());
	int status;
	in.read(&status, sizeof(int));

	switch(status)
	{
	case CStatusMessages::SYNC_STARTING :
		if(fLinkCommand[0] == 0L)
		{
			// This only happens when the daemon tells
			// us to sync based on a button push.
			//
			//
			KConfig& c = KPilotLink::getConfig();
			if (c.readBoolEntry("PreferFastSync",false))
			{
				doFastSync();
			}
			else
			{
				doHotSync();
			}
		}
		fStatusBar->changeItem(i18n("Hot-Sync in progress..."), 0);

		// This block writes some stuff to the link and
		// therefore requires its own ofstream object.
		//
		//
		{
		ofstream out(fPilotCommandSocket->socket());
		out << fLinkCommand << flush;
		}
		break;
	case CStatusMessages::SYNC_COMPLETED :
		fStatusBar->changeItem(i18n("Hot-Sync complete."), 0);
		fLinkCommand[0] = 0L;
		for(fPilotComponentList.first(); 
			fPilotComponentList.current(); 
			fPilotComponentList.next())
		{
			fPilotComponentList.current()->initialize();
		}
		break;
	case CStatusMessages::FILE_INSTALL_REQUEST :
		for(fPilotComponentList.first(); 
			fPilotComponentList.current(); 
			fPilotComponentList.next())
		{
			fPilotComponentList.current()->initialize();
		}
		break;
	default :
		kdWarning() << __FUNCTION__
			<< ": Unknown command "
			<< status
			<< " received."
			<< endl;
	}
}

void
KPilotInstaller::doBackup()
{
	FUNCTIONSETUP;

	fStatusBar->changeItem(
		i18n("Backing up pilot. Please press the hot-sync button."), 
		0);

	sprintf(fLinkCommand, "%d\n", KPilotLink::Backup);
}

void
KPilotInstaller::doRestore()
{
	FUNCTIONSETUP;

  fStatusBar->changeItem(i18n("Restoring pilot. Please press the hot-sync button."), 0);
  sprintf(fLinkCommand, "%d\n", KPilotLink::Restore);
}
 
void KPilotInstaller::setupSync(int kind,const QString& message)
{
	FUNCTIONSETUP;

	showTitlePage(message);

	if (fLinkCommand[0] == 0)
	{
		sprintf(fLinkCommand, "%d\n",kind);
		for(fPilotComponentList.first(); 
			fPilotComponentList.current(); 
			fPilotComponentList.next())
		{
#ifdef DEBUG
			if (debug_level & SYNC_MINOR)
			{
				kdDebug() << fname << ": Pre-sync for builtins."
					<< endl;
			}
#endif
			fPilotComponentList.current()->preHotSync(fLinkCommand);
		}
	}
}


void
KPilotInstaller::closeEvent(QCloseEvent *e)
{
	FUNCTIONSETUP;

  quit();
  e->accept();
}

void
KPilotInstaller::initMenu()
    {
	FUNCTIONSETUP;

    QPopupMenu* fileMenu = new QPopupMenu;
    fileMenu->insertItem(i18n("&Settings"), KPilotInstaller::ID_FILE_SETTINGS);
    fileMenu->insertSeparator(-1);
	fileMenu->insertItem(icon_hotsync,
		i18n("&Hot-Sync"),KPilotInstaller::ID_FILE_HOTSYNC);
	fileMenu->insertItem(icon_fastsync,
		i18n("&Fast-Sync"),KPilotInstaller::ID_FILE_FASTSYNC);
	fileMenu->insertItem(icon_backup,
		i18n("&Backup"), KPilotInstaller::ID_FILE_BACKUP);
	fileMenu->insertItem(icon_restore,
		i18n("&Restore"), KPilotInstaller::ID_FILE_RESTORE);
    fileMenu->insertSeparator(-1);
	fileMenu->insertItem(icon_quit,
		i18n("&Quit"), KPilotInstaller::ID_FILE_QUIT);
    connect(fileMenu, SIGNAL (activated(int)), SLOT (menuCallback(int)));
    
	conduitMenu = new QPopupMenu;
	conduitMenu->insertItem(i18n("&External"), 
		KPilotInstaller::ID_CONDUITS_SETUP);
	conduitMenu->insertSeparator(-1);
	connect(conduitMenu, SIGNAL(activated(int)),
		SLOT(menuCallback(int)));

	KPopupMenu *theHelpMenu = helpMenu();
    
    this->fMenuBar = new KMenuBar(this);
    this->fMenuBar->insertItem(i18n("&File"), fileMenu);
    this->fMenuBar->insertItem(i18n("&Conduits"), conduitMenu);
    this->fMenuBar->insertSeparator();
    this->fMenuBar->insertItem(i18n("&Help"), theHelpMenu);
    this->fMenuBar->show();
    setMenu(this->fMenuBar);
    }

void
KPilotInstaller::fileInstalled(int )
    {
    }

void
KPilotInstaller::quit()
{
	FUNCTIONSETUP;

	for(fPilotComponentList.first(); 
		fPilotComponentList.current(); 
		fPilotComponentList.next())
	{
		fPilotComponentList.current()->saveData();
	}

	if(fKillDaemonOnExit && fPilotCommandSocket &&
		(fPilotCommandSocket->socket()>=0))
	{
#ifdef DEBUG
		if (debug_level & SYNC_TEDIOUS)
		{
			kdDebug() << fname
				<< ": Killing daemon."
				<< endl;
		}
#endif

		ofstream out(fPilotCommandSocket->socket());
		out << -3 << endl;
	}


	if (fPilotStatusSocket) delete fPilotStatusSocket;
	fPilotStatusSocket=0L;

	if (fPilotCommandSocket) delete fPilotCommandSocket;
	fPilotCommandSocket=0L;

	kapp->quit();
}

// Adds 'name' to the pull down menu of components
void
KPilotInstaller::addComponentPage(QWidget* widget, QString name)
{
	FUNCTIONSETUP;

	/*
	fToolBar->insertComboItem(KPilotInstaller::ID_COMBO, name,
		fVisibleWidgetList.count());
	*/
	conduitCombo->insertItem(name,fVisibleWidgetList.count());
	conduitMenu->insertItem(name,
		KPilotInstaller::ID_COMBO+fVisibleWidgetList.count());
	fVisibleWidgetList.append(widget);
}

void KPilotInstaller::menuCallback(int item)
{
	FUNCTIONSETUP;

	KPilotOptions* options = 0L;
	CConduitSetup* conSetup = 0L;

#ifdef DEBUG
	if (debug_level & UI_TEDIOUS)
	{
		kdDebug() << fname << ": Responding to callback " << item
			<< endl;
	}
#endif

	if ((item>=ID_COMBO) && 
		(item<ID_COMBO+(int)fVisibleWidgetList.count()))
	{
		conduitCombo->setCurrentItem(item-ID_COMBO);
		slotModeSelected(item-ID_COMBO);
		return;
	}
		
	switch(item)
	{
	case KPilotInstaller::ID_HELP_ABOUT:
		KMessageBox::information(0L,i18n("Hot-Sync Software for Unix\n"
				"By: Dan Pilone\n"
				"Email: pilone@slac.com"),
				     version(0));
		break;

	case KPilotInstaller::ID_HELP_HELP:
		kapp->invokeHTMLHelp("kpilot/index.html", "");
		break;
	case KPilotInstaller::ID_FILE_QUIT:
		quit();
		break;
	case KPilotInstaller::ID_FILE_SETTINGS:
		// Display the (modal) options page.
		//
		//
		showTitlePage();
		options = new KPilotOptions(this);
		if (options==NULL)
		{
			kdError() << __FUNCTION__ << 
				": Can't allocate KPilotOptions object\n";
			break;
		}

#ifdef DEBUG
		if (debug_level & UI_MINOR)
		{
			kdDebug() << fname << ": Running options dialog." 
				<< endl;
		}
#endif
		options->exec();
#ifdef DEBUG
		if (debug_level & UI_MINOR)
		{
			kdDebug() << fname << ": dialog result "
				<< options->result() << endl;
		}
#endif

		if (options->result())
		{
#ifdef DEBUG
			if (debug_level & UI_TEDIOUS)
			{
				kdDebug() << fname 
					<< ": Updating link." << endl;
			}
#endif

			readConfig(KPilotLink::getConfig());

			// Update the link to reflect new settings.
			//
			//
			destroyPilotLink(); // Get rid of the old one
			initPilotLink(); // make a new one..

			// Update each installed component.
			//
			//
			for(fPilotComponentList.first(); 
			fPilotComponentList.current();
			fPilotComponentList.next())
			{
#ifdef DEBUG
				if (debug_level & UI_TEDIOUS)
				{
					kdDebug() << fname 
						<< ": Updating components." 
						<< endl;
				}
#endif

				fPilotComponentList.current()->initialize();
			}
		}

		delete options;
		options=NULL;
#ifdef DEBUG
		if (debug_level & UI_MINOR)
		{
			kdDebug() << fname << ": Done with options." << endl;
		}
#endif
		break;

	case KPilotInstaller::ID_FILE_BACKUP:
		showTitlePage();
		doBackup();
		break;
	case KPilotInstaller::ID_FILE_RESTORE:
		showTitlePage();
		doRestore();
		break;
	case KPilotInstaller::ID_FILE_HOTSYNC :
		showTitlePage();
		doHotSync();
		break;
	case KPilotInstaller::ID_FILE_FASTSYNC :
		showTitlePage();
		doFastSync();
		break;
	case KPilotInstaller::ID_CONDUITS_SETUP:
		showTitlePage();
		conSetup = new CConduitSetup(this);
		conSetup->exec();
		delete conSetup;
		break;
	}

#ifdef DEBUG
	if (debug_level & UI_TEDIOUS)
	{
		kdDebug() << fname << ": Done responding to item " << item
			<< endl;
	}
#endif
}

void 
KPilotInstaller::slotSyncDone(KProcess*)
{
	FUNCTIONSETUP;

  fStatusBar->changeItem(i18n("Updating display..."), 0);
  kapp->processEvents();
  for(fPilotComponentList.first(); fPilotComponentList.current(); fPilotComponentList.next())
    fPilotComponentList.current()->postHotSync();
  fStatusBar->changeItem(i18n("Hot-Sync complete."),0);
}

 
/* static */ const char *KPilotInstaller::version(int kind)
{
  // I don't think the program title needs to be translated. (ADE)
  //
  //
  if (kind) 
  {
    return ::id;
    }
  else 
  {
    return "KPilot v4.0.1";
    }
}

// Command line options descriptions.
//
//
//
//
static KCmdLineOptions kpilotoptions[] =
{
	{ "setup", I18N_NOOP("Setup the Pilot device and other parameters"),0L },
#ifdef ENABLE_CMD_CS
	{ "cs", I18N_NOOP("Run conduit setup"),0L },
#endif
#ifdef DEBUG
	{ "debug <level>", I18N_NOOP("Set debug level to <level> (try 1023)"),"0" },
#endif
	{ 0,0,0 }
} ;




// "Regular" mode == 0
// setup mode == 's'
//
// This is only changed by the --setup flag --
// kpilot still does a setup the first time it is run.
//
//
int run_mode=0;
			 

int main(int argc, char** argv)
{
	FUNCTIONSETUP;

        KAboutData about("kpilot", I18N_NOOP("KPilot"),
                         "4.0b",
                         "KPilot - Hot-sync software for unix\n\n",
                         KAboutData::License_GPL,
                         "(c) 1998-2000, Dan Pilone");
	about.addAuthor("Dan Pilone",
		I18N_NOOP("Project Leader"),
		"pilone@slac.com",
		"http://www.slac.com/pilone/kpilot_home/");
	about.addAuthor("Adriaan de Groot",
		I18N_NOOP("Maintainer"),
		"adridg@cs.kun.nl",
		"http://www.cs.kun.nl/~adridg/kpilot/");
	about.addAuthor("Heiko Purnhagen",
		I18N_NOOP("Bugfixer"),
		"purnhage@tnt.uni-hannover.de");


        KCmdLineArgs::init(argc, argv, &about);
	KCmdLineArgs::addCmdLineOptions(kpilotoptions);
	KApplication::addCmdLineOptions();
	KCmdLineArgs *p=KCmdLineArgs::parsedArgs();

#ifdef DEBUG
	debug_level=atoi(p->getOption("debug"));
#endif
	if (p->isSet("setup")) { run_mode='s'; } 
#ifdef ENABLE_CMD_CS
	if (p->isSet("cs")) { run_mode='c'; }
#endif

	KApplication a(true,true);

	KConfig& c=KPilotLink::getConfig();
	(void)KPilotLink::getDebugLevel(c);
	if (KPilotLink::getConfigVersion(c)<KPilotLink::ConfigurationVersion)
	{
		run_mode='s';
	}

#ifdef ENABLE_CMD_CS
	if (run_mode=='c')
	{
		CConduitSetup *cs = new CConduitSetup(0L);
		cs->exec();
		exit(2);
	}
#endif

	if (run_mode=='s')
	{
#ifdef DEBUG
		if (debug_level & UI_MAJOR)
		{
			kdDebug() << fname << ": Running setup first."
				<< " (mode " << run_mode << ")"
				<< endl ;
		}
#endif

		KPilotOptions* options = new KPilotOptions(0L);
		options->exec();
		// gsetupDialog uses result 0 for cancel
		//
		if (!options->result()) return 0;

		// The options dialog may have changed the group
		// while reading or writing settings (still a
		// bad idea, actually).
		//
		c.setGroup(QString::null);
	}

	if (KPilotLink::getConfigVersion(c)<KPilotLink::ConfigurationVersion)
	{
		kdWarning() << __FUNCTION__ << ": Is still not configured for use."
			<< endl;
		return 1;
	}

        KPilotInstaller *tp = new KPilotInstaller();

        KGlobal::dirs()->addResourceType("pilotdbs", "share/apps/kpilot/DBBackup");
	a.setMainWidget(tp);
	return a.exec();
}


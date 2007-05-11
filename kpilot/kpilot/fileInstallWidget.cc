/* KPilot
**
** Copyright (C) 1998-2001 by Dan Pilone
**
** This file defines the internal conduit "File Installer"
** that accepts drags of URLs containing Palm DBs, prcs, and
** such. It also does the HotSync part of installing files
** on the Pilot.
*/

/*
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program in a file called COPYING; if not, write to
** the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
** MA 02110-1301, USA.
*/

/*
** Bug reports and questions can be sent to kde-pim@kde.org
*/

#ifndef _KPILOT_OPTIONS_H
#include "options.h"
#endif

#include <unistd.h>

#include <q3listbox.h>

#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>

#include <q3multilineedit.h>
#include <qpixmap.h>
#include <q3popupmenu.h>
//Added by qt3to4:
#include <QDropEvent>
#include <Q3GridLayout>
#include <Q3ValueList>
#include <QMouseEvent>
#include <QEvent>
#include <QDragEnterEvent>

#include <kfiledialog.h>
#include <k3urldrag.h>
#include <kiconloader.h>
#include <k3iconview.h>
#include <kglobal.h>
#include <kurl.h>

#include "kpilotConfig.h"
#include "fileInstaller.h"


#include "fileInstallWidget.moc"

FileInstallWidget::FileInstallWidget(QWidget * parent,
	const QString & path) :
	PilotComponent(parent, "component_files", path),
	fSaveFileList(false),
	fInstaller(0L)
{
	FUNCTIONSETUP;

	Q3GridLayout *grid = new Q3GridLayout(this, 5, 5, SPACING);

	QLabel *label = new QLabel(i18n("Files to install:"), this);

	grid->addWidget(label, 1, 1);

	QPushButton *abutton;

    abutton = addButton = new QPushButton(i18n("Add File..."), this);
	connect(abutton, SIGNAL(clicked()), this, SLOT(slotAddFile()));
	grid->addWidget(abutton, 3, 1);
	abutton->setWhatsThis(
		i18n("<qt>Choose a file to add to the list of files to install.</qt>"));

	abutton = clearButton= new QPushButton(i18n("Clear List"), this);
	connect(abutton, SIGNAL(clicked()), this, SLOT(slotClearButton()));
	grid->addWidget(abutton, 4, 1);
	abutton->setWhatsThis(
		i18n("<qt>Clear the list of files to install. No files will be installed.</qt>"));

	fIconView = new K3IconView(this);
	connect(fIconView, SIGNAL(dropped(QDropEvent *, const Q3ValueList<Q3IconDragItem> &)),
		this, SLOT(slotDropEvent(QDropEvent *, const Q3ValueList<Q3IconDragItem> &)));
	grid->addMultiCellWidget(fIconView, 1, 4, 2, 3);
	fIconView->setWhatsThis(
		i18n
		("<qt>This lists files that will be installed on the Pilot during the next HotSync. Drag files here or use the Add button.</qt>"));
	fIconView->setAcceptDrops(true);
    fIconView->setSelectionMode(Q3IconView::Extended);
	fIconView->viewport()->installEventFilter(this);

	grid->setRowStretch(2, 100);
	grid->setColStretch(2, 50);
	grid->setColStretch(2, 50);
	grid->addColSpacing(4, SPACING);
	grid->addRowSpacing(5, SPACING);

	fInstaller = new FileInstaller;
	connect(fInstaller, SIGNAL(filesChanged()),
		this, SLOT(refreshFileInstallList()));

}

FileInstallWidget::~FileInstallWidget()
{
	KPILOT_DELETE(fInstaller);
}

static inline bool pdbOrPrc(const QString &s)
{
	return s.endsWith(CSL1(".pdb"),false) || s.endsWith(CSL1(".prc"),false) ;
}

void FileInstallWidget::dragEnterEvent(QDragEnterEvent *event)
{
	FUNCTIONSETUP;

	KUrl::List urls;
	if(!K3URLDrag::decode(event, urls)) {
		event->accept(false);
		return;
	}

	KUrl::List::const_iterator it;
	QString filename;
    for ( it = urls.begin(); it != urls.end(); ++it ) {
		filename = (*it).fileName();
		if(!pdbOrPrc(filename)) {
			event->accept(false);
			return;
		}
	}
	event->accept(true);
}

bool FileInstallWidget::eventFilter(QObject *watched, QEvent *event)
{
	FUNCTIONSETUP;

    if(watched == fIconView->viewport())
    {
        if(event->type() == QEvent::DragEnter) {
    		dragEnterEvent(static_cast<QDragEnterEvent*>(event));
            return true;
        }

        // We have to skip the DragMove event, because it seems to override the
        // accept state, when it is set to false by dragEnterEvent() (event->accept(false);)
        if(event->type() == QEvent::DragMove) {
            return true;
        }

        if(event->type() == QEvent::MouseButtonPress) {
            contextMenu(static_cast<QMouseEvent*>(event));
        }
    }

	return false;
}

void FileInstallWidget::dropEvent(QDropEvent * drop)
{
	FUNCTIONSETUP;
	if (!isVisible()) return;

	KUrl::List list;

	if (!K3URLDrag::decode(drop, list) || list.isEmpty())
		return;

#ifdef DEBUG
	DEBUGKPILOT << ": Got " << list.first().prettyUrl() << endl;
#endif

	QStringList files;
	for(KUrl::List::ConstIterator it = list.begin(); it != list.end(); ++it)
	{
	   if ((*it).isLocalFile())
	      files << (*it).path();
	}

	fInstaller->addFiles(files, this );
}

void FileInstallWidget::slotDropEvent(QDropEvent * drop, const Q3ValueList<Q3IconDragItem> & /*lst*/)
{
	FUNCTIONSETUP;
	dropEvent(drop);
}

void FileInstallWidget::slotClearButton()
{
	FUNCTIONSETUP;
	fInstaller->clearPending();
}

void FileInstallWidget::showComponent()
{
	FUNCTIONSETUP;
	refreshFileInstallList();
}

void FileInstallWidget::slotAddFile()
{
	FUNCTIONSETUP;
	if (!isVisible()) return;

	QStringList fileNames = KFileDialog::getOpenFileNames(
		KUrl(), i18n("*.pdb *.prc|PalmOS Databases (*.pdb *.prc)"));

	for (QStringList::Iterator fileName = fileNames.begin(); fileName != fileNames.end(); ++fileName)
	{
		fInstaller->addFile(*fileName, this );
	}
}

bool FileInstallWidget::preHotSync(QString &)
{
	FUNCTIONSETUP;

	fIconView->setEnabled(false);
	fInstaller->setEnabled(false);
	addButton->setEnabled(false);
	clearButton->setEnabled(false);

	return true;
}

void FileInstallWidget::postHotSync()
{
	FUNCTIONSETUP;

	fInstaller->setEnabled(true);
	fIconView->setEnabled(true);
	addButton->setEnabled(true);
	clearButton->setEnabled(true);
	if (isVisible()) refreshFileInstallList();
}


void FileInstallWidget::refreshFileInstallList()
{
	FUNCTIONSETUP;

	QStringList fileNames = fInstaller->fileNames();
	QPixmap kpilotIcon = KIconLoader::global()->loadIcon(CSL1("kpilot"), K3Icon::Desktop);

	fIconView->clear();

	for (QStringList::Iterator fileName = fileNames.begin(); fileName != fileNames.end(); ++fileName)
	{
		if(pdbOrPrc(*fileName))
		{
			new K3IconViewItem(fIconView, *fileName, kpilotIcon);
		}
		else
		{
			new K3IconViewItem(fIconView, *fileName);
		}
	}
}

void FileInstallWidget::contextMenu(QMouseEvent *event)
{
    FUNCTIONSETUP;

    if(event->button() == Qt::LeftButton)
        return;

    Q3IconViewItem *item;
    QStringList files;
    for(item = fIconView->firstItem(); item; item = item->nextItem())
    {
        if(item->isSelected())
            files.append(item->text());
    }

    Q3PopupMenu popup(fIconView);

    item = fIconView->findItem(event->pos());
    if(item) {
        // Popup for the right clicked item
        popup.insertItem(i18nc("Delete a single file item","Delete"), 10);
    }

    popup.insertItem(i18n("Delete selected files"), 11);
    if(files.empty())
        popup.setItemEnabled(11, false);

    int id = popup.exec(fIconView->viewport()->mapToGlobal(event->pos()));
    if(id == 10)
        fInstaller->deleteFile(item->text());
    else if(id == 11)
        fInstaller->deleteFiles(files);

}

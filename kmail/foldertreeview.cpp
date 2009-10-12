/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009 Montel Laurent <montel@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "foldertreeview.h"
#include <QDebug>
#include <KLocale>

FolderTreeView::FolderTreeView(QWidget *parent )
  : Akonadi::EntityTreeView( parent )
{
  init();
}


FolderTreeView::FolderTreeView(KXMLGUIClient *xmlGuiClient, QWidget *parent )
  :Akonadi::EntityTreeView( xmlGuiClient, parent )
{
  init();
}


FolderTreeView::~FolderTreeView()
{
}

void FolderTreeView::init()
{
}

void FolderTreeView::selectModelIndex( const QModelIndex & index )
{
  if ( index.isValid() ) {
    clearSelection();
    scrollTo( index );
    setCurrentIndex(index);
  }
}


void FolderTreeView::slotFocusNextFolder()
{
  QModelIndex current = currentIndex();
  if ( current.isValid() ) {
    model()->fetchMore( current );
    if ( model()->hasChildren( current ) ) {
      expand( current );
      QModelIndex below = indexBelow( current );
      selectModelIndex( below );
    } else if ( current.row() < model()->rowCount( model()->parent( current ) ) -1 ) {
      QModelIndex item = model()->index( current.row()+1, current.column(), model()->parent( current ) );
      selectModelIndex( item );
    } else {
      QModelIndex below = indexBelow( current );
      selectModelIndex( below );
    }
  }
}

#include "foldertreeview.moc"

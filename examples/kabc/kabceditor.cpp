/*
    This file is part of Akonadi.

    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "kabceditor.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include <akonadi/contact/contacteditor.h>
#include <akonadi/item.h>
#include <kapplication.h>
#include <kcmdlineargs.h>

Dialog::Dialog( QWidget *parent )
  : KDialog( parent )
{
  setCaption( i18n("Contact Editor") );
  setButtons( Close );

  showButtonSeparator( true );

  QWidget *wdg = new QWidget( this );
  QGridLayout *layout = new QGridLayout( wdg );

  mEditor = new Akonadi::ContactEditor( Akonadi::ContactEditor::EditMode, wdg );
  layout->addWidget( mEditor, 0, 0, 1, 3 );

  QLabel *label = new QLabel( i18n("Item Id:"), wdg );
  layout->addWidget( label, 1, 0 );

  mId = new QLineEdit( wdg );
  layout->addWidget( mId, 1, 1 );

  QPushButton *button = new QPushButton( i18n("Load"), wdg );
  layout->addWidget( button, 1, 2 );

  connect( button, SIGNAL(clicked()), SLOT(load()) );

  button = new QPushButton( i18n("Save"), wdg );
  layout->addWidget( button, 2, 2 );

  connect( button, SIGNAL(clicked()), SLOT(save()) );

  setMainWidget( wdg );
}

Dialog::~Dialog()
{
}

void Dialog::load()
{
  mEditor->loadContact( Akonadi::Item( mId->text().toLongLong() ) );
}

void Dialog::save()
{
  mEditor->saveContact();
}

int main( int argc, char **argv )
{
  KCmdLineArgs::init( argc, argv, "kabceditor", 0, ki18n("KABC Editor"), "1.0" , ki18n("A contact editor for Akonadi"));
  KApplication app;

  Dialog dlg;
  dlg.exec();

  return 0;
}


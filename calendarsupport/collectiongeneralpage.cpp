/*
  Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
    Author: Tobias Koenig <tokoe@kdab.com>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

#include "collectiongeneralpage.h"

#include <Akonadi/Collection>
#include <Akonadi/EntityDisplayAttribute>
#include <akonadi/calendar/blockalarmsattribute.h>

#include <KCalCore/Event>
#include <KCalCore/Journal>
#include <KCalCore/Todo>

#include <KDialog>
#include <KIconButton>
#include <KLineEdit>
#include <KLocalizedString>

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

using namespace Akonadi;
using namespace CalendarSupport;

CollectionGeneralPage::CollectionGeneralPage( QWidget *parent )
  : CollectionPropertiesPage( parent )
{
  setObjectName( QLatin1String( "CalendarSupport::CollectionGeneralPage" ) );
  setPageTitle( i18nc( "@title:tab General settings for a folder.", "General" ) );

  QVBoxLayout *topLayout = new QVBoxLayout( this );
  topLayout->setSpacing( KDialog::spacingHint() );
  topLayout->setMargin( 0 );

  QHBoxLayout *hbox = new QHBoxLayout();
  topLayout->addItem( hbox );
  hbox->setSpacing( KDialog::spacingHint() );

  QLabel *label = new QLabel( i18nc( "@label:textbox Name of the folder.", "&Name:" ), this );
  hbox->addWidget( label );

  mNameEdit = new KLineEdit( this );
  mNameEdit->setToolTip(
    i18nc( "@info:tooltip", "Set the folder name" ) );
  mNameEdit->setWhatsThis(
    i18nc( "@info:whatsthis",
           "Enter a name here to set the name of this folder." ) );
  label->setBuddy( mNameEdit );
  hbox->addWidget( mNameEdit );

  // should replies to mails in this folder be kept in this same folder?
  hbox = new QHBoxLayout();
  topLayout->addItem( hbox );
  hbox->setSpacing( KDialog::spacingHint() );

  mBlockAlarmsCheckBox = new QCheckBox( i18nc( "@option:check", "Block reminders locally" ), this );
  mBlockAlarmsCheckBox->setToolTip(
    i18nc( "@info:tooltip", "Ignore reminders from this calendar" ) );
  mBlockAlarmsCheckBox->setWhatsThis(
    i18nc( "@info:whatsthis",
           "Check this box if you do not want to receive reminders from items "
           "associated with this calendar." ) );
  hbox->addWidget( mBlockAlarmsCheckBox );
  hbox->addStretch( 1 );

#ifndef KDEPIM_MOBILE_UI
  hbox = new QHBoxLayout();
  topLayout->addItem( hbox );
  hbox->setSpacing( KDialog::spacingHint() );
  mIconCheckBox = new QCheckBox( i18nc( "@option:check", "&Use custom icon:" ), this );
  mIconCheckBox->setToolTip(
    i18nc( "@info:tooltip", "Set a custom icon" ) );
  mIconCheckBox->setWhatsThis(
    i18nc( "@info:whatsthis",
           "Check this box if you want to set a custom icon for this folder." ) );
  mIconButton = new KIconButton( this );
  mIconButton->setIconSize( 16 );
  hbox->addWidget( mIconCheckBox );
  hbox->addWidget( mIconButton );
  hbox->addStretch();
#endif

  topLayout->addStretch( 100 ); // eat all superfluous space
}

CollectionGeneralPage::~CollectionGeneralPage()
{
}

void CollectionGeneralPage::load( const Akonadi::Collection &collection )
{
  mNameEdit->setEnabled( collection.rights() & Collection::CanChangeCollection );

  const QString displayName = collection.displayName();

  mNameEdit->setText( displayName );
  mBlockAlarmsCheckBox->setChecked( collection.hasAttribute<BlockAlarmsAttribute>() && collection.attribute<BlockAlarmsAttribute>()->isEverythingBlocked() );

  QString iconName;
  if ( collection.hasAttribute<EntityDisplayAttribute>() ) {
    iconName = collection.attribute<EntityDisplayAttribute>()->iconName();
  }

#ifndef KDEPIM_MOBILE_UI
  if ( iconName.isEmpty() ) {
    const QStringList mimeTypes = collection.contentMimeTypes();
    if ( collection.contentMimeTypes().count() > 1 ||
         collection.contentMimeTypes().contains( KCalCore::Event::eventMimeType() ) ) {
      mIconButton->setIcon( QLatin1String("view-pim-calendar") );
    } else if ( collection.contentMimeTypes().contains( KCalCore::Todo::todoMimeType() ) ) {
      mIconButton->setIcon( QLatin1String("view-pim-tasks") );
    } else if ( collection.contentMimeTypes().contains( KCalCore::Journal::journalMimeType() ) ) {
      mIconButton->setIcon( QLatin1String("view-pim-journal") );
    } else if ( mimeTypes.isEmpty() ) {
      mIconButton->setIcon( QLatin1String("folder-grey") );
    } else {
      mIconButton->setIcon( QLatin1String("folder") );
    }
  } else {
    mIconButton->setIcon( iconName );
  }
  mIconCheckBox->setChecked( !iconName.isEmpty() );
#endif
}

void CollectionGeneralPage::save( Collection &collection )
{
  if ( collection.hasAttribute<EntityDisplayAttribute>() &&
       !collection.attribute<EntityDisplayAttribute>()->displayName().isEmpty() ) {
    collection.attribute<EntityDisplayAttribute>()->setDisplayName( mNameEdit->text() );
  } else {
    collection.setName( mNameEdit->text() );
  }

  if ( !collection.hasAttribute<BlockAlarmsAttribute>() ) {
      collection.attribute<BlockAlarmsAttribute>( Collection::AddIfMissing );
  }

  if ( mBlockAlarmsCheckBox->isChecked() ) {
      collection.attribute<BlockAlarmsAttribute>()->blockEverything(true);
  } else {
      collection.attribute<BlockAlarmsAttribute>()->blockEverything(false);
  }

#ifndef KDEPIM_MOBILE_UI
  if ( mIconCheckBox->isChecked() ) {
    collection.attribute<EntityDisplayAttribute>( Collection::AddIfMissing )->
      setIconName( mIconButton->icon() );
  } else if ( collection.hasAttribute<EntityDisplayAttribute>() ) {
    collection.attribute<EntityDisplayAttribute>()->setIconName( QString() );
  }
#endif

}


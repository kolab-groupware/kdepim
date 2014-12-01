/*
  This file is part of KOrganizer.

  Copyright (c) 2001,2003 Cornelius Schumacher <schumacher@kde.org>
  Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

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

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "koprefs.h"

#include <KGlobalSettings>

#include <QDir>

class KOPrefsPrivate
{
  public:
    KOPrefsPrivate() : prefs( new KOPrefs ) {}
    ~KOPrefsPrivate() { delete prefs; }
    KOPrefs *prefs;
};

K_GLOBAL_STATIC( KOPrefsPrivate, sInstance )

KOPrefs::KOPrefs() : KOPrefsBase()
{
  KGlobal::locale()->insertCatalog( QLatin1String("calendarsupport") );

  mEventViewsPrefs = EventViews::PrefsPtr( new EventViews::Prefs( this ) );

  mDefaultMonthViewFont = KGlobalSettings::generalFont();
  // make it a bit smaller
  mDefaultMonthViewFont.setPointSize(
    qMax( mDefaultMonthViewFont.pointSize() - 2, 6 ) );

  KConfigSkeleton::setCurrentGroup( QLatin1String("General") );

  // writes into mHtmlExportFile
  addItemPath( QLatin1String("Html Export File"), mHtmlExportFile,
               QDir::homePath() + QLatin1Char('/') + i18nc( "Default export file", "calendar.html" ) );

  monthViewFontItem()->setDefaultValue( mDefaultMonthViewFont );
}

KOPrefs::~KOPrefs()
{
  kDebug();
  mEventViewsPrefs->writeConfig();
}

KOPrefs *KOPrefs::instance()
{
  if ( !sInstance.exists() ) {
    sInstance->prefs->readConfig();
    sInstance->prefs->mEventViewsPrefs->readConfig();
  }

  return sInstance->prefs;
}

void KOPrefs::usrSetDefaults()
{
  setMonthViewFont( mDefaultMonthViewFont );

  KConfigSkeleton::usrSetDefaults();
}

void KOPrefs::usrReadConfig()
{
  KConfigGroup generalConfig( config(), "General" );

  KConfigGroup timeScaleConfig( config(), "Timescale" );
  setTimeScaleTimezones( timeScaleConfig.readEntry( "Timescale Timezones", QStringList() ) );

  KConfigSkeleton::usrReadConfig();
}

void KOPrefs::usrWriteConfig()
{
  KConfigGroup generalConfig( config(), "General" );

  KConfigGroup timeScaleConfig( config(), "Timescale" );
  timeScaleConfig.writeEntry( "Timescale Timezones", timeScaleTimezones() );

  KConfigSkeleton::usrWriteConfig();
}

void KOPrefs::setResourceColor ( const QString &cal, const QColor &color )
{
  return mEventViewsPrefs->setResourceColor( cal, color );
}

QColor KOPrefs::resourceColor( const QString &cal )
{
  return mEventViewsPrefs->resourceColor( cal );
}

QColor KOPrefs::resourceColorKnown(const QString &cal)
{
  return mEventViewsPrefs->resourceColorKnown( cal );
}

QStringList KOPrefs::timeScaleTimezones() const
{
  return mTimeScaleTimeZones;
}

void KOPrefs::setTimeScaleTimezones( const QStringList &list )
{
  mTimeScaleTimeZones = list;
}

void KOPrefs::setHtmlExportFile( const QString &fileName )
{
  mHtmlExportFile = fileName;
}

QString KOPrefs::htmlExportFile() const
{
  return mHtmlExportFile;
}

EventViews::PrefsPtr KOPrefs::eventViewsPreferences() const
{
  return mEventViewsPrefs;
}

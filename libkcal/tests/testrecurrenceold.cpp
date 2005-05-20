/*
    This file is part of libkcal.

    Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>
    Copyright (C) 2005 Reinhold Kainhofer <reinhold@kainhofe.com>

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
*/

#include "recurrencerule.h"

#include "icalformat.h"
#include "icalformatimpl.h"
#include "calendarlocal.h"

#include <kaboutdata.h>
#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kcmdlineargs.h>

#include <qfile.h>
#include <qfileinfo.h>

#include <iostream>



using namespace KCal;


static const KCmdLineOptions options[] =
{
  { "verbose", "Verbose output", 0 },
  { "+input", "Name of input file", 0 },
  KCmdLineLastOption
};


int main( int argc, char **argv )
{
  KAboutData aboutData( "testrecurrencenew", "Load recurrence rules with the new class and print out debug messages", "0.1" );
  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineArgs::addCmdLineOptions( options );

  KApplication app( false, false );

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  if ( args->count() < 1 ) {
    args->usage( "Wrong number of arguments." );
  }

  QString input = QFile::decodeName( args->arg( 0 ) );
  kdDebug(5800) << "Input file: " << input << endl;

  QTextStream *outstream;
  outstream = 0;
  QString fn("");
  if ( args->count() > 1 ) {
    fn = args->arg( 1 );
    kdDebug() << "We have a file name given: " << fn << endl;
  }
  QFile outfile( fn );
  if ( !fn.isEmpty() && outfile.open( IO_WriteOnly ) ) {
    kdDebug() << "Opened output file!!!" << endl;
    outstream = new QTextStream( &outfile );
  }

  CalendarLocal cal( QString::fromLatin1("UTC") );

  if ( !cal.load( input ) ) return 1;

  Incidence::List inc = cal.incidences();
/*
  for ( Incidence::List::Iterator it = inc.begin(); it != inc.end(); ++it ) {
    Incidence *incidence = *it;
    kdDebug(5800) << "*+*+*+*+*+*+*+*+*+*" << endl;
    kdDebug(5800) << " -> " << incidence->summary() << " <- " << endl;

    QPtrList<RecurrenceRule> rrules = incidence->recurrence()->getRecurrenceRules();
    if ( !rrules.isEmpty() ) {
      RecurrenceRule *recur = rrules.first();
      recur->dump();

    // QDateTime::currentDateTime()
      QDateTime dt( recur->startDate().addSecs(-2) );
      int i=0;
      if ( outstream ) {
        // Output to file for testing purposes
        while (dt.isValid() && i<500 ) {
          ++i;
          dt = dt.addSecs( 1 );
          dt = recur->getNextDate( dt );
          (*outstream) << dt.toString( Qt::ISODate ) << endl;
        }
      } else {
        // Output to konsole
        for ( i = 0; i<10; ++i ) {
          kdDebug(5800) << "-------------------------------------------" << endl;
          dt = recur->getNextDate( dt );
          kdDebug(5800) << " *~*~*~*~ Next date is: " << dt << endl;
          dt = dt.addSecs( 1 );
        }
      }
    }
  }*/


  // OLD way to get the recurrences
  for ( Incidence::List::Iterator it = inc.begin(); it != inc.end(); ++it ) {
    Incidence *incidence = *it;
    kdDebug(5800) << "*+*+*+*+*+*+*+*+*+*" << endl;
    kdDebug(5800) << " -> " << incidence->summary() << " <- " << endl;

    // QDateTime::currentDateTime()
    QDateTime dt( incidence->dtStart().addSecs(-2) );
    int i=0;
    if ( outstream ) {
      // Output to file for testing purposes
      while (dt.isValid() && i<500 ) {
        ++i;
        dt = dt.addSecs( 1 );
        dt = incidence->recurrence()->getNextDateTime( dt );
        (*outstream) << dt.toString( Qt::ISODate ) << endl;
      }
    }
  }

//   cal.save( "ffff.ics" );



	delete outstream;
  outfile.close();
  return 0;
}

/*  -*- mode: C++; c-file-style: "gnu" -*-
    text_vcard.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2004 Till Adam <adam@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

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

#include <QUrl>

#include <kapplication.h>
#include <kglobal.h>
#include <klocale.h>
#include <kstringhandler.h>
#include <kglobalsettings.h>
#include <kiconloader.h>

#include <akonadi/contact/standardcontactformatter.h>

#include <libkdepim/addcontactjob.h>

#include "messageviewer/interfaces/bodypartformatter.h"
#include "messageviewer/interfaces/bodypart.h"
using MessageViewer::Interface::BodyPart;
#include "messageviewer/interfaces/bodyparturlhandler.h"
#include "messageviewer/webkitparthtmlwriter.h"
#include <kimproxy.h>

#include <kabc/vcardconverter.h>
#include <kabc/addressee.h>
using KABC::VCardConverter;
using KABC::Addressee;

#include <kdemacros.h>

namespace {

  class Formatter : public MessageViewer::Interface::BodyPartFormatter {
  public:
    Formatter() {
      // disabled pending resolution of how to share static objects when dlopening libraries
      //mKIMProxy = ::KIMProxy::instance( kapp->dcopClient() );
    }

    Result format( BodyPart *bodyPart, MessageViewer::HtmlWriter *writer ) const {

       if ( !writer ) return AsIcon;

       VCardConverter vcc;
       const QString vCard = bodyPart->asText();
       if ( vCard.isEmpty() ) return AsIcon;
       Addressee::List al = vcc.parseVCards( vCard.toUtf8() );
       if ( al.empty() ) return AsIcon;

       writer->queue (
             "<div align=\"center\"><h2>" +
             i18n( "Attached business cards" ) +
             "</h2></div>"
                );

       int count = 0;
       foreach (const KABC::Addressee& a, al ) {
          if ( a.isEmpty() ) return AsIcon;

          Akonadi::StandardContactFormatter formatter;
          formatter.setContact( a );

          writer->queue( formatter.toHtml( Akonadi::StandardContactFormatter::EmbeddableForm ) );

          QString addToLinkText = i18n( "[Add this contact to the address book]" );
          QString op = QString::fromLatin1( "addToAddressBook:%1" ).arg( count );
          writer->queue(
                "<div align=\"center\"><a href=\"" +
                bodyPart->makeLink( op ) +
                "\">" +
                addToLinkText +
                "</a></div><br><br>" );
          count++;
       }

       return Ok;
    }
  private:
    //::KIMProxy *mKIMProxy;
};

  class UrlHandler : public MessageViewer::Interface::BodyPartURLHandler {
  public:
     bool handleClick(  MessageViewer::Viewer* /*viewerInstance*/, BodyPart * bodyPart, const QString & path ) const {

       const QString vCard = bodyPart->asText();
       if ( vCard.isEmpty() ) return true;
       VCardConverter vcc;
       Addressee::List al = vcc.parseVCards( vCard.toUtf8() );
       int index = path.right( path.length() - path.lastIndexOf( ":" ) - 1 ).toInt();
       if ( index == -1 || index >= al.count() ) return true;
       KABC::Addressee a = al[index];
       if ( a.isEmpty() ) return true;

       KPIM::AddContactJob *job = new KPIM::AddContactJob( a, 0 );
       job->start();

       return true;
     }

     bool handleContextMenuRequest(  BodyPart *, const QString &, const QPoint & ) const {
       return false;
     }

     QString statusBarMessage(  BodyPart *, const QString & ) const {
       return i18n("Add this contact to the address book.");
     }
  };

  class Plugin : public MessageViewer::Interface::BodyPartFormatterPlugin {
  public:
    const MessageViewer::Interface::BodyPartFormatter * bodyPartFormatter( int idx ) const {
      return validIndex( idx ) ? new Formatter() : 0 ;
    }
    const char * type( int idx ) const {
      return validIndex( idx ) ? "text" : 0 ;
    }
    const char * subtype( int idx ) const {
      switch( idx ) {
        case 0: return "x-vcard";
        case 1: return "vcard";
        case 2: return "directory";
        default: return 0;
      }
    }

    const MessageViewer::Interface::BodyPartURLHandler * urlHandler( int idx ) const {
       return validIndex( idx ) ? new UrlHandler() : 0 ;
    }
  private:
    bool validIndex( int idx ) const {
      return ( idx >= 0 && idx <= 2 );
    }
  };

}

extern "C"
KDE_EXPORT MessageViewer::Interface::BodyPartFormatterPlugin *
messageviewer_bodypartformatter_text_vcard_create_bodypart_formatter_plugin() {
  KGlobal::locale()->insertCatalog( "messageviewer_text_vcard_plugin" );
  return new Plugin();
}


/*
  Requires the Qt and KDE widget libraries, available at no cost at
  http://www.trolltech.com and http://www.kde.org respectively

  Copyright (c) 2002-2004 Klarälvdalens Datakonsult AB
        <info@klaralvdalens-datakonsult.se>

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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
  MA  02110-1301, USA.

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

#include "groupware.h"
#include "calendaradaptor.h"
#include "kcalprefs.h"
#include "mailscheduler.h"

#include <KCalUtils/IncidenceFormatter>
#include <KCalUtils/Stringify>

#include <KLocale>
#include <KMessageBox>

using namespace CalendarSupport;

Groupware *Groupware::mInstance = 0;

GroupwareUiDelegate::~GroupwareUiDelegate()
{
}

Groupware *Groupware::create( CalendarSupport::Calendar *calendar, GroupwareUiDelegate *delegate )
{
  if ( !mInstance ) {
    mInstance = new Groupware( calendar, delegate );
  }
  return mInstance;
}

Groupware *Groupware::instance()
{
  // Doesn't create, that is the task of create()
  Q_ASSERT( mInstance );
  return mInstance;
}

Groupware::Groupware( CalendarSupport::Calendar *cal, GroupwareUiDelegate *delegate )
  : QObject( 0 ), mCalendar( cal ), mDelegate( delegate ), mDoNotNotify( false )
{
  setObjectName( QLatin1String( "kmgroupware_instance" ) );
}

bool Groupware::handleInvitation( const QString &receiver, const QString &iCal,
                                  const QString &type )
{
  const QString action = type;

  CalendarAdaptor::Ptr adaptor( new CalendarAdaptor( mCalendar, 0 ) );
  KCalCore::ScheduleMessage *message = mFormat.parseScheduleMessage( adaptor, iCal );
  if ( !message ) {
    QString errorMessage;
    if ( mFormat.exception() ) {
      errorMessage =
        i18n( "Error message: %1", KCalUtils::Stringify::errorMessage( *mFormat.exception() ) );
    }
    kDebug() << "Error parsing" << errorMessage;
    KMessageBox::detailedError(
      0,
      i18n( "Error while processing an invitation or update." ),
      errorMessage );
    return false;
  }

  KCalCore::iTIPMethod method = static_cast<KCalCore::iTIPMethod>( message->method() );
  KCalCore::ScheduleMessage::Status status = message->status();
  KCalCore::Incidence::Ptr incidence = message->event().staticCast<KCalCore::Incidence>();
  if ( !incidence ) {
    delete message;
    return false;
  }
  MailScheduler scheduler( mCalendar );
  if ( action.startsWith( QLatin1String( "accepted" ) ) ||
       action.startsWith( QLatin1String( "tentative" ) ) ||
       action.startsWith( QLatin1String( "delegated" ) ) ||
       action.startsWith( QLatin1String( "counter" ) ) ) {
    // Find myself and set my status. This can't be done in the scheduler,
    // since this does not know the choice I made in the KMail bpf
    KCalCore::Attendee::List attendees = incidence->attendees();
    KCalCore::Attendee::List::ConstIterator it;
    for ( it = attendees.constBegin(); it != attendees.constEnd(); ++it ) {
      if ( (*it)->email() == receiver ) {
        if ( action.startsWith( QLatin1String( "accepted" ) ) ) {
          (*it)->setStatus( KCalCore::Attendee::Accepted );
        } else if ( action.startsWith( QLatin1String( "tentative" ) ) ) {
          (*it)->setStatus( KCalCore::Attendee::Tentative );
        } else if ( KCalPrefs::instance()->outlookCompatCounterProposals() &&
                    action.startsWith( QLatin1String( "counter" ) ) ) {
          (*it)->setStatus( KCalCore::Attendee::Tentative );
        } else if ( action.startsWith( QLatin1String( "delegated" ) ) ) {
          (*it)->setStatus( KCalCore::Attendee::Delegated );
        }
        break;
      }
    }
    if ( KCalPrefs::instance()->outlookCompatCounterProposals() ||
         !action.startsWith( QLatin1String( "counter" ) ) ) {
      scheduler.acceptTransaction( incidence, method, status, receiver );
    }
  } else if ( action.startsWith( QLatin1String( "cancel" ) ) ) {
    // Delete the old incidence, if one is present
    scheduler.acceptTransaction( incidence, KCalCore::iTIPCancel, status, receiver );
  } else if ( action.startsWith( QLatin1String( "reply" ) ) ) {
    if ( method != KCalCore::iTIPCounter ) {
      scheduler.acceptTransaction( incidence, method, status, QString() );
    } else {
      // accept counter proposal
      scheduler.acceptCounterProposal( incidence );
      // send update to all attendees
      sendICalMessage( 0, KCalCore::iTIPRequest, incidence,
                       IncidenceChanger::INCIDENCEEDITED, false );
    }
  } else {
    kError() << "Unknown incoming action" << action;
  }

  if ( mDelegate && action.startsWith( QLatin1String( "counter" ) ) ) {
    Akonadi::Item item;
    item.setPayload( KCalCore::Incidence::Ptr( incidence->clone() ) );
    mDelegate->requestIncidenceEditor( item );
  }
  delete message;
  return true;
}

class KOInvitationFormatterHelper : public KCalUtils::InvitationFormatterHelper
{
  public:
    virtual QString generateLinkURL( const QString &id )
    {
      return QLatin1String( "kmail:groupware_request_" ) + id;
    }
};

/* This function sends mails if necessary, and makes sure the user really
 * want to change his calendar.
 *
 * Return true means accept the changes
 * Return false means revert the changes
 */
bool Groupware::sendICalMessage( QWidget *parent,
                                 KCalCore::iTIPMethod method,
                                 const KCalCore::Incidence::Ptr &incidence,
                                 IncidenceChanger::HowChanged action,
                                 bool attendeeStatusChanged )
{
  // If there are no attendees, don't bother
  if ( incidence->attendees().isEmpty() ) {
    return true;
  }

  bool isOrganizer = KCalPrefs::instance()->thatIsMe( incidence->organizer()->email() );
  int rc = 0;
  /*
   * There are two scenarios:
   * o "we" are the organizer, where "we" means any of the identities or mail
   *   addresses known to Kontact/PIM. If there are attendees, we need to mail
   *   them all, even if one or more of them are also "us". Otherwise there
   *   would be no way to invite a resource or our boss, other identities we
   *   also manage.
   * o "we: are not the organizer, which means we changed the completion status
   *   of a todo, or we changed our attendee status from, say, tentative to
   *   accepted. In both cases we only mail the organizer. All other changes
   *   bring us out of sync with the organizer, so we won't mail, if the user
   *   insists on applying them.
   */

  if ( isOrganizer ) {
    /* We are the organizer. If there is more than one attendee, or if there is
     * only one, and it's not the same as the organizer, ask the user to send
     * mail. */
    if ( incidence->attendees().count() > 1 ||
         incidence->attendees().first()->email() != incidence->organizer()->email() ) {

      QString txt;
      switch( action ) {
      case IncidenceChanger::INCIDENCEEDITED:
        txt = i18n( "You changed the invitation \"%1\".\n"
                    "Do you want to email the attendees an update message?",
                    incidence->summary() );
        break;
      case IncidenceChanger::INCIDENCEDELETED:
        Q_ASSERT( incidence->type() == KCalCore::IncidenceBase::TypeEvent ||
                  incidence->type() == KCalCore::IncidenceBase::TypeTodo );
        if ( incidence->type() == KCalCore::IncidenceBase::TypeEvent ) {
          txt = i18n( "You removed the invitation \"%1\".\n"
                      "Do you want to email the attendees that the event is canceled?",
                      incidence->summary() );
        } else if ( incidence->type() == KCalCore::IncidenceBase::TypeTodo ) {
          txt = i18n( "You removed the invitation \"%1\".\n"
                      "Do you want to email the attendees that the todo is canceled?",
                      incidence->summary() );
        }
        break;
      case IncidenceChanger::INCIDENCEADDED:
        if ( incidence->type() == KCalCore::IncidenceBase::TypeEvent ) {
          txt = i18n( "The event \"%1\" includes other people.\n"
                      "Do you want to email the invitation to the attendees?",
                      incidence->summary() );
        } else if ( incidence->type() == KCalCore::IncidenceBase::TypeTodo ) {
          txt = i18n( "The todo \"%1\" includes other people.\n"
                      "Do you want to email the invitation to the attendees?",
                      incidence->summary() );
        } else {
          txt = i18n( "This incidence includes other people. "
                      "Should an email be sent to the attendees?" );
        }
        break;
      default:
        kError() << "Unsupported HowChanged action" << int( action );
        break;
      }

      rc = KMessageBox::questionYesNo(
             parent, txt, i18n( "Group Scheduling Email" ),
             KGuiItem( i18n( "Send Email" ) ), KGuiItem( i18n( "Do Not Send" ) ) );
    } else {
      return true;
    }
  } else if ( incidence->type() == KCalCore::IncidenceBase::TypeTodo ) {
    if ( method == KCalCore::iTIPRequest ) {
      // This is an update to be sent to the organizer
      method = KCalCore::iTIPReply;
    }
    // Ask if the user wants to tell the organizer about the current status
    QString txt = i18n( "Do you want to send a status update to the "
                        "organizer of this task?" );
    rc = KMessageBox::questionYesNo(
           parent, txt, QString(),
           KGuiItem( i18n( "Send Update" ) ), KGuiItem( i18n( "Do Not Send" ) ) );
  } else if ( incidence->type() == KCalCore::IncidenceBase::TypeEvent ) {
    QString txt;
    if ( attendeeStatusChanged && method == KCalCore::iTIPRequest ) {
      txt = i18n( "Your status as an attendee of this event changed. "
                  "Do you want to send a status update to the event organizer?" );
      method = KCalCore::iTIPReply;
      rc = KMessageBox::questionYesNo(
             parent, txt, QString(),
             KGuiItem( i18n( "Send Update" ) ), KGuiItem( i18n( "Do Not Send" ) ) );
    } else {
      if ( action == IncidenceChanger::INCIDENCEDELETED ) {
        const QStringList myEmails = KCalPrefs::instance()->allEmails();
        bool askConfirmation = false;
        for ( QStringList::ConstIterator it = myEmails.begin(); it != myEmails.end(); ++it ) {
          QString email = *it;
          KCalCore::Attendee::Ptr me = incidence->attendeeByMail(email);
          if ( me &&
               ( me->status() == KCalCore::Attendee::Accepted ||
                 me->status() == KCalCore::Attendee::Delegated ) ) {
            askConfirmation = true;
            break;
          }
        }

        if ( !askConfirmation ) {
          return true;
        }

        txt = i18n( "You had previously accepted an invitation to this event. "
                    "Do you want to send an updated response to the organizer "
                    "declining the invitation?" );
        rc = KMessageBox::questionYesNo(
          parent, txt, i18n( "Group Scheduling Email" ),
          KGuiItem( i18n( "Send Update" ) ), KGuiItem( i18n( "Do Not Send" ) ) );
        setDoNotNotify( rc == KMessageBox::No );
      } else if ( action == IncidenceChanger::INCIDENCEADDED ) {
        // We just got this event from the groupware stack, so add it right away
        // the notification mail was sent on the KMail side.
        return true;
      } else {
        txt = i18n( "You are not the organizer of this event. Editing it will "
                    "bring your calendar out of sync with the organizer's calendar. "
                    "Do you really want to edit it?" );
        rc = KMessageBox::warningYesNo( parent, txt );
        return rc == KMessageBox::Yes;
      }
    }
  } else {
    kWarning() << "Groupware messages for Journals are not implemented yet!";
    return true;
  }

  if ( rc == KMessageBox::Yes ) {
    // We will be sending out a message here. Now make sure there is
    // some summary
    if ( incidence->summary().isEmpty() ) {
      incidence->setSummary( i18n( "<placeholder>No summary given</placeholder>" ) );
    }
    // Send the mail
    MailScheduler scheduler( mCalendar );
    if ( scheduler.performTransaction( incidence, method ) ) {
      return true;
    }
    rc = KMessageBox::questionYesNo(
           parent,
           i18n( "Sending group scheduling email failed." ),
           i18n( "Group Scheduling Email" ),
           KGuiItem( i18n( "Abort Update" ) ), KGuiItem( i18n( "Do Not Send" ) ) );
    return rc == KMessageBox::No;
  } else if ( rc == KMessageBox::No ) {
    return true;
  } else {
    return false;
  }
}

void Groupware::sendCounterProposal( KCalCore::Event::Ptr oldEvent,
                                     KCalCore::Event::Ptr newEvent ) const
{
  if ( !oldEvent || !newEvent || *oldEvent == *newEvent ||
       !KCalPrefs::instance()->mUseGroupwareCommunication ) {
    return;
  }
  if ( KCalPrefs::instance()->outlookCompatCounterProposals() ) {
    KCalCore::Incidence::Ptr tmp = KCalCore::Incidence::Ptr( oldEvent->clone() );
    tmp->setSummary( i18n( "Counter proposal: %1", newEvent->summary() ) );
    tmp->setDescription( newEvent->description() );
    tmp->addComment( i18n( "Proposed new meeting time: %1 - %2",
                           KCalUtils::IncidenceFormatter::dateToString( newEvent->dtStart() ),
                           KCalUtils::IncidenceFormatter::dateToString( newEvent->dtEnd() ) ) );
    MailScheduler scheduler( mCalendar );
    scheduler.performTransaction( tmp, KCalCore::iTIPReply );
  } else {
    MailScheduler scheduler( mCalendar );
    scheduler.performTransaction( newEvent, KCalCore::iTIPCounter );
  }
}

#include "groupware.moc"

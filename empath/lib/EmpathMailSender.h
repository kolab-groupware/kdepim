/*
	Empath - Mailer for KDE
	
	Copyright (C) 1998 Rik Hemsley rik@kde.org
	
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
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef EMPATHMAILSENDER_H
#define EMPATHMAILSENDER_H

// Qt includes
#include <qobject.h>

// Local includes
#include "RMM_Message.h"
#include "EmpathDefines.h"
#include "EmpathMessageList.h"

/**
 * Base class for any 'real' sender. This one's only responsibility
 * is to queue messages in a local spool for later delivery by a
 * derived class. (well _that_ makes sense ;)
 */
class EmpathMailSender : public QObject
{
	Q_OBJECT

	public:

		EmpathMailSender();

		virtual ~EmpathMailSender() = 0L;

		/**
		 * Send one message.
		 * 
		 * @returns false if the message could not be delivered.
		 *
		 * Message will be returned to user on failure. FIXME: How ?
		 */
		virtual bool sendOne(RMessage & message) = 0L;

		/**
		 * Send a batch of messages. This could be useful if there's
		 * some sorting logic put into the derived classes. i.e. if you're
		 * sending to an SMTP host, you can sometimes send all messages
		 * in one connection, saving time and bandwidth. This is certainly
		 * the case for people with a dedicated mail server (er - the world ?)
		 * that stores and despatches mail on their behalf. I know my old isp
		 * did this, so I presume a lot of other peoples' isps do it too.
		 */
		virtual bool send(EmpathMessageList & messageList) = 0L;

		/**
		 * This will add a message to the local queue, i.e. spool it.
		 * It will be despatched when the derived class feels like it.
		 */
		void addPendingMessage(RMessage & message);

		virtual void saveConfig() = 0;
		virtual void readConfig() = 0;

	private:

		QList<RMessage>	pendingList_;
};

#endif


/* kpilotDCOP.h			KPilotDaemon
**
** Copyright (C) 2000 by Adriaan de Groot
**
** This file defines the DCOP interface for KPilot.
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
** the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, 
** MA 02139, USA.
*/

/*
** Bug reports and questions can be sent to adridg@cs.kun.nl
*/

#ifndef _KPILOT_KPILOTDCOP_H
#define _KPILOT_KPILOTDCOP_H

#include <dcopobject.h>


class KPilotDCOP : virtual public DCOPObject
{
	K_DCOP
k_dcop:
	virtual ASYNC filesChanged() = 0;
	virtual ASYNC daemonStatus(QString) = 0;
	virtual ASYNC daemonProgress(QString,int) = 0;
} ;

#else
#ifdef DEBUG
#warning "File doubly included"
#endif
#endif


// $Log$
// Revision 1.3  2001/04/16 13:48:35  adridg
// --enable-final cleanup and #warning reduction
//
// Revision 1.2  2001/03/09 09:46:15  adridg
// Large-scale #include cleanup
//
// Revision 1.1  2001/03/04 22:22:29  adridg
// DCOP cooperation between daemon & kpilot for d&d file install
//

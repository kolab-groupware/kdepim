/*
 *   This file only:
 *     Copyright (C) 2004  Mark Bucciarelli <mark@hubcapconsulting.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the
 *      Free Software Foundation, Inc.
 *      59 Temple Place - Suite 330
 *      Boston, MA  02111-1307  USA.
 */
#ifndef KARM_DCOP_IFAC_H
#define KARM_DCOP_IFAC_H

#include <dcopobject.h>

class KarmDCOPIface : virtual public DCOPObject
{
  K_DCOP
  k_dcop:

  /** Return karm version. */
  virtual QString version() const = 0;

  virtual QString setStorage( const QString& storage ) = 0;
};

#endif // KARM_DCOP_IFAC_H

/*
  Copyright 2009 Olivier Trichet <nive@nivalis.org>

  Permission to use, copy, modify, and distribute this software
  and its documentation for any purpose and without fee is hereby
  granted, provided that the above copyright notice appear in all
  copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#ifndef KNODE_IDENTITYCOMBOWRAPPER_H
#define KNODE_IDENTITYCOMBOWRAPPER_H

#include "knglobals.h"

#include <KPIMIdentities/IdentityCombo>


namespace KNode {

/**
  Wrapper around KPIMIdentities::IdentityCombo to make available a constructor
  with a QWidget as single argument. This constructor is needed by the autogenerated UI class
  from identity_widget.ui.
*/
class IdentityComboWrapper : public KPIMIdentities::IdentityCombo
{
  public:
    explicit IdentityComboWrapper( QWidget *parent = 0 )
      : KPIMIdentities::IdentityCombo( KNGlobals::self()->identityManager(), parent )
    {
    };
};

}

#endif

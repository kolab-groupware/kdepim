/*
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2012 Montel Laurent <montel@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "filterimporterabstract_p.h"
#include "mailfilter.h"
#include "filtermanager.h"

using namespace MailCommon;

FilterImporterAbstract::FilterImporterAbstract()
{
}

FilterImporterAbstract::~FilterImporterAbstract()
{
}

QList<MailFilter*> FilterImporterAbstract::importFilter() const
{
    return mListMailFilter;
}


void FilterImporterAbstract::createFilterAction(MailCommon::MailFilter *filter, const QString& actionName, const QString& value)
{
    if ( !actionName.isEmpty() ) {
      FilterActionDesc *desc = MailCommon::FilterManager::filterActionDict()->value( actionName );
      if ( desc ) {
        FilterAction *fa = desc->create();
        //...create an instance...
        fa->argsFromStringInteractive( value, filter->name() );
        //...check if it's empty and...
        if ( !fa->isEmpty() )
          //...append it if it's not and...
          filter->actions()->append( fa );
        else
          //...delete is else.
          delete fa;
      }
    }
}

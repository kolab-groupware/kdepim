/*
    Empath - Mailer for KDE
    
    Copyright 1999, 2000
        Rik Hemsley <rik@kde.org>
        Wilco Greven <j.w.greven@student.utwente.nl>
    
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

#ifdef __GNUG__
# pragma interface "EmpathMessageHTMLView.h"
#endif

#ifndef EMPATHMESSAGEHTMLVIEW_H
#define EMPATHMESSAGEHTMLVIEW_H

// Qt includes
#include <qstring.h>
#include <qpopupmenu.h>
#include <qtextbrowser.h>

// Local includes
#include "EmpathURL.h"

class EmpathMessageHTMLWidget : public QTextBrowser
{
    Q_OBJECT

    public:
        
        EmpathMessageHTMLWidget(QWidget *);
        ~EmpathMessageHTMLWidget();
        
        void show(const QString & xml);
        virtual QSize sizeHint() const;
        virtual QSize minimumSizeHint() const;
        
    protected slots:
        
        void s_popupMenu(QString, const QPoint &);

    private:
        
        QPopupMenu popup_;
};

#endif
// vim:ts=4:sw=4:tw=78

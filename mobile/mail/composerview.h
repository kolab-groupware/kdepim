/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef COMPOSERVIEW_H
#define COMPOSERVIEW_H

#include "kdeclarativefullscreenview.h"

#include <KActionCollection>

class KJob;

namespace KPIMIdentities
{
  class IdentityCombo;
}

namespace Message
{
  class KMeditor;
}

/** The new KMMainWidget ;-) */
class ComposerView : public KDeclarativeFullScreenView
{
  Q_OBJECT
  Q_PROPERTY( QString subject READ subject WRITE setSubject )

  public:
    explicit ComposerView(QWidget* parent = 0);

    void setIdentityCombo( KPIMIdentities::IdentityCombo* combo ) { m_identityCombo = combo; }
    void setEditor( Message::KMeditor* editor ) { m_editor = editor; }

    QString subject() const;
    void setSubject( const QString &subject );

    KActionCollection* actionCollection() const;

  public slots:
    /// Send clicked in the user interface
    void send();
    QObject* getAction( const QString &name ) const;

  private slots:
    void qmlLoaded ( QDeclarativeView::Status );
    void composerResult( KJob* job );
    void sendResult( KJob* job );

  private:
    KPIMIdentities::IdentityCombo *m_identityCombo;
    Message::KMeditor *m_editor;
    QString m_subject;
    KActionCollection *mActionCollection;
};

#endif

/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Stephen Kelly <stephen@kdab.com>

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

import QtQuick 1.1 as QML
import org.kde.pim.mobileui 4.5 as KPIM
import org.kde 4.5

QML.Rectangle {
  anchors.fill : parent
  anchors.topMargin : 12
  property alias backgroundImage : backgroundImage.source

  QML.Image {
    id: backgroundImage
    x: 0
    y: 0
  }

  signal finished()
  signal canceled()

  KPIM.MultipleSelectionComponent {
    id : multSelectionComponent
    anchors.top : parent.top
    anchors.left : parent.left
    anchors.right : parent.right
    anchors.bottom : buttonRow.top
  }

  QML.Item {
    id : buttonRow
    anchors.bottom : parent.bottom
    anchors.left : parent.left
    anchors.right : parent.right
    height : 50
    KPIM.Button2 {
      id : doneButton
      anchors.top : parent.top
      anchors.bottom : parent.bottom
      anchors.left : parent.left
      buttonText : KDE.i18n("Done")
      width : 150
      onClicked :
      {
        finished();
        guiStateManager.popState();
        application.clearPersistedSelection("preFavSelection");
        application.multipleSelectionFinished();
      }
    }
    KPIM.Button2 {
      id : cancelButton
      anchors.top : parent.top
      anchors.bottom : parent.bottom
      anchors.right : parent.right
      buttonText : KDE.i18n("Cancel")
      width : 150
      onClicked :
      {
        canceled();
        guiStateManager.popState();
        application.restorePersistedSelection("preFavSelection");
      }
    }
  }
}

/*
 *  Copyright 2013 (C) Michael Bohlender <michael.bohlender@kdemail.net>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of
 *  the License or (at your option) version 3 or any later version
 *  accepted by the membership of KDE e.V. (or its successor approved
 *  by the membership of KDE e.V.), which shall act as a proxy
 *  defined in Section 14 of version 3 of the license.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 1.1
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.pim.mobileui 4.5 as KPIM

PlasmaComponents.Page {
  id: root

  implicitWidth: pageRow.width * 2 /3

  //BEGIN: Tools
  tools: PlasmaComponents.ToolBarLayout {
    PlasmaComponents.ToolButton {
      anchors.left: parent.left
      iconSource: "go-previous"
      onClicked: pageRow.pop()
    }
  }
  //END: Tools

  KPIM.ReorderListContainer {
    anchors.fill: parent

    model : favoritesList
    actionItemHeight: root.height * 0.2

    KPIM.ActionButton {
      icon : "images/moveup.png"
      actionName : "favoriteseditor_moveup"
    }

    KPIM.ActionButton {
      icon : "images/movedown.png"
      actionName : "favoriteseditor_movedown"
    }

    KPIM.ActionButton {
      icon : KDE.locate( "data", "mobileui/delete-button.png" )
      actionName : "favoriteseditor_remove"
    }

    onCurrentIndexChanged: favoritesEditor.setRowSelected( index )
  }
}
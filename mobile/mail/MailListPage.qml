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
import org.kde.plasma.extras 0.1 as PlasmaExtras

PlasmaComponents.Page {
  id: root

  property variant navigationModel: _threadSelector
  property variant checkModel: _itemActionModel

  function mailStatusIconSource(mail) {
    if (mail.is_important) {
      return "mail-mark-important"
    } else if (mail.is_replied) {
      return "mail-replied"
    } else if (mail.is_forwarded) {
      return "mail-forwarded"
    } else if (mail.is_unread) {
      return "mail-unread-new"
    } else {
      return "mail-read"
    }
  }

  implicitWidth: pageRow.width * 2 /3

  //BEGIN Tools
  tools: PlasmaComponents.ToolBarLayout {

    PlasmaComponents.ToolButton {
      iconSource: "go-previous"

      onClicked: pageRow.pop()
    }

    //TODO (de)select-all checkbox
    Row {

      anchors.horizontalCenter: parent.horizontalCenter

      spacing: root.width * 0.03

      PlasmaComponents.ToolButton {
        iconSource: "mail-mark-unread"

        enabled: checkModel.hasSelection

        onClicked: application.getAction("akonadi_mark_as_read", "").trigger()
      }

      PlasmaComponents.ToolButton {
        iconSource: "mail-mark-important"

        enabled: checkModel.hasSelection

        onClicked: application.getAction("akonadi_mark_as_important", "").trigger()
      }

      //TODO usability feature: offer to undo deletion
      PlasmaComponents.ToolButton {
        iconSource: "edit-delete"

        enabled: checkModel.hasSelection

        onClicked: application.getAction("akonadi_move_to_trash", "").trigger()
      }
    }


    //TODO add new mail from template once the multiple actions button is ready
    PlasmaComponents.ToolButton {

      anchors.right: parent.right

      iconSource: "mail-message-new"

      onClicked: application.startComposer()
    }
  }
  //END Tools

  ListView {
    id : threadView

    anchors.fill: parent

    property int currentItemId: -1
    property int currentRow : -1

    model: _threads

    focus: true
    clip: true
    currentIndex: -1


    onCurrentRowChanged: {
      if (navigationModel != undefined)
        navigationModel.select(currentRow, 3)
    }

    Connections {
      target : navigationModel
      onCurrentRowChanged: currentRow = navigationModel.currentRow
    }

    //BEGIN Delegate
    delegate: PlasmaComponents.ListItem {
      id: headerListDelegate

      height: label.height * 2.5

      opacity: model.is_important || model.is_unread ? 1 : 0.65
      clip: true
      enabled: true
      checked: threadView.currentIndex == index

      onClicked: {
        if (root == pageRow.currentPage) {
          pageRow.push(Qt.resolvedUrl("MailViewPage.qml"))
        }
        threadView.currentIndex = index
        navigationModel.select(model.index, 3)
      }

      onPressAndHold: threadView.currentIndex = index

      Rectangle {
        id: itemBackground

        anchors.fill: parent
        color: checked ? "lightgrey" : "white"
        opacity: 0.5
      }

      PlasmaComponents.CheckBox {
        id: checkBox

        anchors {
          left: parent.left
          leftMargin: label.width
          verticalCenter: parent.verticalCenter
        }

        visible: root == pageRow.currentPage
        checked: model.checkOn

        onClicked: checkModel.select(model.index, 8)
      }

      Image {
        id: avatar

        anchors {
          left: root == pageRow.currentPage ? checkBox.right : parent.left
          leftMargin: label.width
          verticalCenter: parent.verticalCenter
        }

        height: parent.height * 0.7
        width: height

        source: "dummy-avatar.png"
        fillMode: Image.PreserveAspectFit
        smooth: true
      }

      PlasmaComponents.Label {
        id: fromLabel

        anchors {
          top : parent.top
          left : avatar.right
          leftMargin: label.width
          right: dateLabel.left
        }

        text : model.from
        elide: "ElideRight"
        font.weight: Font.Light
        color : "#0C55BB"
      }

      PlasmaComponents.Label {
        id: dateLabel

        anchors {
          top: parent.top
          right: statusIcon.left
        }

        text: model.date
        horizontalAlignment: "AlignRight"
        font.weight: Font.Light
        color : "#0C55BB"
      }

      PlasmaExtras.Heading {
        id: subjectLabel

        anchors {
          bottom: parent.bottom
          left: avatar.right
          leftMargin: label.width
          right: statusIcon.left
        }

        level: 4
        text: model.subject
        elide: "ElideRight"
        color: "#3B3B3B"
      }

      PlasmaComponents.ToolButton {
        id: statusIcon

        anchors{
          right: parent.right
          verticalCenter: parent.verticalCenter
        }

        height: parent.height * 0.7

        iconSource: mailStatusIconSource(model)

        onClicked: dialog.open()

        //BEGIN Dialog
        PlasmaComponents.Dialog {
          id: dialog

          visualParent: parent

          buttons: Column {

            spacing: root.width * 0.03

            PlasmaComponents.ToolButton {

              iconSource: "mail-mark-unread"

              onClicked: {
                checkModel.select(model.index, 3)
                application.getAction("akonadi_mark_as_read", "").trigger()
                checkModel.select(-1, 1)
              }

            }

            PlasmaComponents.ToolButton {

              iconSource: "mail-mark-important"

              onClicked: {
                checkModel.select(model.index, 3)
                application.getAction("akonadi_mark_as_important", "").trigger()
                checkModel.select(-1, 1)
              }

            }

            PlasmaComponents.ToolButton {

              iconSource: "edit-delete"

              onClicked: {
                checkModel.select(model.index, 3)
                application.getAction("akonadi_move_to_trash", "").trigger()
                checkModel.select(-1, 1)
              }

            }
          }
        }
        //END Dialog
      }
    }
    //END Delegate
  }

  PlasmaComponents.Label {
    id: label

    text: "   "
  }
}
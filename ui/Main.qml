import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Dialogs
import Gbemu


ApplicationWindow {
    id: window
    width: FrontendQt.screenWidth
    height: FrontendQt.screenHeight + menuBar.height
    visible: true

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            Action {
                text: qsTr("&Load ROM...")
                onTriggered: romDialog.open()
            }
            MenuSeparator { }
            Action {
                text: qsTr("&Save State")
                shortcut: "F1"
                onTriggered: FrontendQt.save_state()
            }
            Action {
                text: qsTr("&Load State")
                shortcut: "F2"
                onTriggered: FrontendQt.load_state()
            }
            MenuSeparator { }
            Action {
                text: qsTr("&Quit")
                onTriggered: Qt.quit()
            }
        }
        Menu {
            title: qsTr("&Screen")

            ActionGroup {
                id: scaleGroup
                exclusive: true
            }

            Action {
                text: qsTr("x1")
                checkable: true
                checked: FrontendQt.screenScale === 1
                ActionGroup.group: scaleGroup
                onTriggered: FrontendQt.set_scale(1)
            }
            Action {
                text: qsTr("x2")
                checkable: true
                checked: FrontendQt.screenScale === 2
                ActionGroup.group: scaleGroup
                onTriggered: FrontendQt.set_scale(2)
            }
            Action {
                text: qsTr("x3")
                checkable: true
                checked: FrontendQt.screenScale === 3
                ActionGroup.group: scaleGroup
                onTriggered: FrontendQt.set_scale(3)
            }
        }
        Menu {
            title: qsTr("&Help")
            Action {
                text: qsTr("&About")
                onTriggered: aboutDialog.open()
            }
        }
    }
    GBFramebufferItem {
        frame: FrontendQt.frame
        width:  FrontendQt.screenWidth
        height: FrontendQt.screenHeight
        anchors.centerIn: parent
    }

    FileDialog {
        id: romDialog
        title: qsTr("Load ROM")
        nameFilters: [qsTr("Game Boy ROMs (*.gb)")]
        onAccepted: FrontendQt.load_rom(selectedFile.toString())
    }

    AboutDialog {
        id: aboutDialog
    }
}
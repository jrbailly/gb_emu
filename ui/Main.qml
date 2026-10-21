import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Dialogs
import Gbemu


ApplicationWindow {
    id: window
    width: FrontendQt.screenWidth
    height: FrontendQt.screenHeight + menuBar.height

    minimumWidth: FrontendQt.screenWidth
    maximumWidth: FrontendQt.screenWidth
    minimumHeight: FrontendQt.screenHeight + menuBar.height
    maximumHeight: FrontendQt.screenHeight + menuBar.height
    flags: Qt.Window | Qt.MSWindowsFixedSizeDialogHint
    visible: true

    menuBar: MenuBar {
        Menu {
            title: qsTr("File")
            Action {
                text: qsTr("Load ROM...")
                onTriggered: romDialog.open()
            }
            MenuSeparator { }
            Action {
                text: qsTr("Save State")
                shortcut: "F1"
                onTriggered: FrontendQt.save_state()
            }
            Action {
                text: qsTr("Load State")
                shortcut: "F2"
                onTriggered: FrontendQt.load_state()
            }
            MenuSeparator { }
            Action {
                text: qsTr("Quit")
                onTriggered: Qt.quit()
            }
        }
        Menu {
            title: qsTr("Screen")

            Menu {
                title: qsTr("Scale")

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

            MenuSeparator { }

            Menu {
                title: qsTr("Language")

                ActionGroup {
                    id: languageGroup
                    exclusive: true
                }

                Action {
                    text: "Français"
                    checkable: true
                    checked: FrontendQt.language === "fr"
                    ActionGroup.group: languageGroup
                    onTriggered: FrontendQt.set_language("fr")
                }
                Action {
                    text: "English"
                    checkable: true
                    checked: FrontendQt.language === "en"
                    ActionGroup.group: languageGroup
                    onTriggered: FrontendQt.set_language("en")
                }
            }
        }
        Menu {
            title: qsTr("Controller")

            Action {
                text: qsTr("Keyboard")
                onTriggered: keyBindingsDialog.open()
            }
        }
        Menu {
            title: qsTr("Help")
            Action {
                text: qsTr("About")
                onTriggered: aboutDialog.open()
            }
        }
    }
    Shortcut {
        sequence: "Esc"
        onActivated: Qt.quit()
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
        onAccepted: FrontendQt.load_rom(selectedFile)
    }

    AboutDialog {
        id: aboutDialog
    }

    KeyBindingsDialog {
        id: keyBindingsDialog
    }
}
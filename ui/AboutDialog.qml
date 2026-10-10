import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: aboutDialog

    title: qsTr("About")
    anchors.centerIn: parent
    modal: true
    standardButtons: Dialog.Ok

    ColumnLayout {
        spacing: 8

        Label {
            text: Qt.application.name
            font.bold: true
            font.pointSize: 16
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: qsTr("Game Boy DMG emulator")
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: Qt.application.version
            Layout.alignment: Qt.AlignHCenter
        }
    }
}

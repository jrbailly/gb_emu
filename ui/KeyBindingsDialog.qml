import QtQuick
import QtQuick.Controls
import QtQuick.Layouts


Dialog {
    id: keyBindingsDialog

    title: qsTr("Keyboard Bindings")
    anchors.centerIn: parent
    modal: true
    standardButtons: Dialog.Close
    padding: 10

    ColumnLayout {
        spacing: 6

        Repeater {
            model: ControllersQt.keyBindings

            delegate: RowLayout {
                spacing: 10

                Label {
                    text: modelData.controller
                    Layout.preferredWidth: 70
                }

                Button {
                    Layout.preferredWidth: 200
                    text: ControllersQt.rebindingKey === modelData.keyCode
                          ? qsTr("Press a key...")
                          : modelData.keyText
                    highlighted: ControllersQt.rebindingKey === modelData.keyCode
                    onClicked: ControllersQt.rebindingKey = modelData.keyCode
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: window
    visible: true

    Material.background: "#292e42"

    header: ToolBar {
        Material.background: "#1f2335"

        Label {
            anchors.centerIn: parent
            text: "󰠮 Notes book"
            color: "#a9b1d6"
            font.pixelSize: 20
            elide: Label.ElideRight
        }
    }

    StackLayout {
        id: mainView

        StackLayout {
            id: notesList
        }
    }

    RoundButton {
        id: fab
        implicitHeight: fab.size
        implicitWidth: fab.size
        highlighted: true

        text: ""
        font.pixelSize: 32

        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 20

        Material.foreground: "#ff007c"

        property double size: 64.0
    }
}

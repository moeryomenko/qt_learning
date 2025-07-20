import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    color: "#202225"

    property int selectedServer: 0

    Column {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        ServerIcon {
            id: homeButton
            width: 48
            height: 48
            anchors.horizontalCenter: parent.horizontalCenter
            iconName: "home"
            backgroundColor: "#5865f2"
            isSelected: selectedServer === 0
            onClicked: selectedServer = 0
        }

        Rectangle {
            width: 32
            height: 2
            anchors.horizontalCenter: parent.horizontalCenter
            color: "#4f545c"
            radius: 1
        }

        Repeater {
            model: [
                {
                    name: "Server 1",
                    color: "#ff6b6b",
                    initial: "S1"
                },
                {
                    name: "Server 2",
                    color: "#4ecdc4",
                    initial: "S2"
                },
                {
                    name: "Gaming Hub",
                    color: "#45b7d1",
                    initial: "GH"
                },
                {
                    name: "Dev Team",
                    color: "#f9ca24",
                    initial: "DT"
                },
                {
                    name: "Friends",
                    color: "#6c5ce7",
                    initial: "FR"
                }
            ]

            ServerIcon {
                width: 48
                height: 48
                anchors.horizontalCenter: parent.horizontalCenter
                iconText: modelData.initial
                backgroundColor: modelData.color
                isSelected: selectedServer === index + 1
                onClicked: selectedServer = index + 1

                ToolTip.visible: hovered
                ToolTip.text: modelData.name
                ToolTip.delay: 500
            }
        }

        Item {
            width: 1
            height: 8
        }

        ServerIcon {
            width: 48
            height: 48
            anchors.horizontalCenter: parent.horizontalCenter
            iconName: "add"
            backgroundColor: "#36393f"
            isAddButton: true
            onClicked: console.log("Add server clicked")

            ToolTip.visible: hovered
            ToolTip.text: "Add a Server"
            ToolTip.delay: 500
        }

        ServerIcon {
            width: 48
            height: 48
            anchors.horizontalCenter: parent.horizontalCenter
            iconName: "explore"
            backgroundColor: "#36393f"
            onClicked: console.log("Explore servers clicked")

            ToolTip.visible: hovered
            ToolTip.text: "Explore Public Servers"
            ToolTip.delay: 500
        }
    }
}

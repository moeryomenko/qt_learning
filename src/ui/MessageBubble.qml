import QtQuick

Rectangle {
    id: root
    width: parent.width
    height: isGrouped ? contentText.height + 8 : avatarSize + 16
    color: hovered ? "#32353b" : "transparent"

    property string username: ""
    property string timestamp: ""
    property string content: ""
    property string avatar: "#7289da"
    property bool isGrouped: false
    property bool hovered: mouseArea.containsMouse
    property int avatarSize: 40

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.top: parent.top
        anchors.topMargin: isGrouped ? 2 : 8
        spacing: 16

        Rectangle {
            id: avatarContainer
            width: avatarSize
            height: avatarSize
            radius: avatarSize / 2
            color: avatar
            visible: !isGrouped

            Text {
                anchors.centerIn: parent
                text: username.length > 0 ? username.charAt(0).toUpperCase() : "?"
                color: "#ffffff"
                font.pixelSize: 16
                font.weight: Font.Bold
            }
        }

        Item {
            width: avatarSize
            height: avatarSize
            visible: isGrouped

            Text {
                visible: hovered
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: timestamp.split(" ")[3] || ""
                color: "#72767d"
                font.pixelSize: 11
            }
        }

        Column {
            spacing: 2
            width: root.width - avatarSize - 48

            Row {
                spacing: 8
                visible: !isGrouped

                Text {
                    text: username
                    color: "#ffffff"
                    font.pixelSize: 15
                    font.weight: Font.Medium
                }

                Text {
                    text: timestamp
                    color: "#72767d"
                    font.pixelSize: 12
                    anchors.baseline: parent.children[0].baseline
                }
            }

            Text {
                id: contentText
                text: content
                color: "#dcddde"
                font.pixelSize: 14
                wrapMode: Text.Wrap
                width: parent.width
                lineHeight: 1.3
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
    }

    Row {
        visible: hovered
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.top: parent.top
        anchors.topMargin: 8
        spacing: 4

        IconButton {
            iconName: "emoji"
            size: 20
            onClicked: console.log("Add reaction")
        }

        IconButton {
            iconName: "reply"
            size: 20
            onClicked: console.log("Reply")
        }

        IconButton {
            iconName: "more"
            size: 20
            onClicked: console.log("More options")
        }
    }

    Behavior on color {
        ColorAnimation {
            duration: 150
            easing.type: Easing.OutCubic
        }
    }
}

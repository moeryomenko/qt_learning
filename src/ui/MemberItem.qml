import QtQuick

Rectangle {
    id: root
    width: parent.width
    height: hasActivity ? 44 : 34
    color: hovered ? "#35383e" : "transparent"

    property string username: ""
    property string discriminator: ""
    property string status: "offline"
    property string activity: ""
    property string avatar: "#7289da"
    property string role: ""
    property string roleColor: "#99aab5"
    property bool hovered: mouseArea.containsMouse
    property bool hasActivity: activity !== ""

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        Rectangle {
            id: avatarContainer
            width: 32
            height: 32
            radius: 16
            color: avatar

            Text {
                anchors.centerIn: parent
                text: username.length > 0 ? username.charAt(0).toUpperCase() : "?"
                color: "#ffffff"
                font.pixelSize: 14
                font.weight: Font.Bold
            }

            Rectangle {
                id: statusIndicator
                width: 10
                height: 10
                radius: 5
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.bottomMargin: -1
                anchors.rightMargin: -1
                border.width: 2
                border.color: "#2f3136"

                color: {
                    switch (status) {
                    case "online":
                        return "#43b581";
                    case "idle":
                        return "#faa61a";
                    case "dnd":
                        return "#f04747";
                    case "offline":
                    default:
                        return "#747f8d";
                    }
                }
            }
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 0

            Row {
                spacing: 0

                Text {
                    text: username
                    color: roleColor
                    font.pixelSize: 14
                    font.weight: Font.Medium
                }

                Text {
                    text: discriminator
                    color: "#8e9297"
                    font.pixelSize: 14
                }
            }

            Text {
                visible: hasActivity
                text: activity
                color: "#b9bbbe"
                font.pixelSize: 12
                width: Math.min(implicitWidth, 150)
                elide: Text.ElideRight
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor

        onClicked: console.log("Member clicked:", username)

        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onPressed: function (mouse) {
            if (mouse.button === Qt.RightButton) {
                console.log("Right-clicked on member:", username);
            }
        }
    }

    Behavior on color {
        ColorAnimation {
            duration: 150
            easing.type: Easing.OutCubic
        }
    }
}

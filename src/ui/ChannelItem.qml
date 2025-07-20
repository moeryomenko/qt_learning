import QtQuick

Rectangle {
    id: root
    width: parent.width
    height: 34
    color: isSelected ? "#393c43" : (hovered ? "#35383e" : "transparent")

    property string channelName: ""
    property string channelType: "text"
    property bool isSelected: false
    property bool hasUnread: false
    property int mentionCount: 0
    property int userCount: 0
    property bool hovered: mouseArea.containsMouse

    signal clicked

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 6

        SvgIcon {
            iconName: channelType === "text" ? "hash" : "volume"
            iconColor: isSelected ? "#ffffff" : "#8e9297"
            size: 16
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: channelName
            color: hasUnread ? "#ffffff" : (isSelected ? "#ffffff" : "#8e9297")
            font.pixelSize: 14
            font.weight: hasUnread ? Font.Medium : Font.Normal
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            visible: channelType === "voice" && userCount > 0
            text: userCount.toString()
            color: "#8e9297"
            font.pixelSize: 12
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4

        Rectangle {
            visible: mentionCount > 0
            width: Math.max(16, mentionText.width + 8)
            height: 16
            radius: 8
            color: "#f04747"

            Text {
                id: mentionText
                anchors.centerIn: parent
                text: mentionCount > 99 ? "99+" : mentionCount.toString()
                color: "#ffffff"
                font.pixelSize: 10
                font.weight: Font.Bold
            }
        }

        Rectangle {
            visible: hasUnread && mentionCount === 0
            width: 8
            height: 8
            radius: 4
            color: "#ffffff"
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    Behavior on color {
        ColorAnimation {
            duration: 150
            easing.type: Easing.OutCubic
        }
    }
}

import QtQuick

Column {
    id: root
    width: parent.width
    spacing: 0

    property string categoryName: ""
    property bool expanded: true
    property var channels: []
    property int selectedChannel: -1

    signal channelClicked(int index)

    Rectangle {
        id: categoryHeader
        width: parent.width
        height: 24
        color: "transparent"

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4

            SvgIcon {
                iconName: expanded ? "chevron-down" : "chevron-right"
                iconColor: "#72767d"
                size: 10
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: categoryName
                color: "#72767d"
                font.pixelSize: 12
                font.weight: Font.Bold
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: expanded = !expanded
        }
    }

    Column {
        width: parent.width
        visible: expanded
        spacing: 1

        Repeater {
            model: channels

            ChannelItem {
                width: parent.width
                channelName: modelData.name
                channelType: modelData.type
                isSelected: root.selectedChannel === index
                hasUnread: modelData.unread || false
                mentionCount: modelData.mentions || 0
                userCount: modelData.users || 0

                onClicked: root.channelClicked(index)
            }
        }
    }
}

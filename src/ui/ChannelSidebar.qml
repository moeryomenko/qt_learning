import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    color: "#2f3136"

    property string serverName: "General Server"
    property int selectedChannel: 0

    Column {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: serverHeader
            width: parent.width
            height: 48
            color: "#27292d"

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                Text {
                    text: serverName
                    color: "#ffffff"
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    anchors.verticalCenter: parent.verticalCenter
                }

                SvgIcon {
                    iconName: "chevron-down"
                    iconColor: "#b9bbbe"
                    size: 12
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: "#202225"
            }
        }

        ScrollView {
            width: parent.width
            height: parent.height - serverHeader.height - userPanel.height
            clip: true

            Column {
                width: parent.width
                spacing: 0

                ChannelCategory {
                    categoryName: "TEXT CHANNELS"
                    expanded: true
                    channels: [
                        {
                            name: "general",
                            type: "text",
                            unread: false,
                            mentions: 0
                        },
                        {
                            name: "random",
                            type: "text",
                            unread: true,
                            mentions: 2
                        },
                        {
                            name: "announcements",
                            type: "text",
                            unread: false,
                            mentions: 0
                        }
                    ]
                    onChannelClicked: function (index) {
                        selectedChannel = index;
                    }
                    selectedChannel: root.selectedChannel
                }

                ChannelCategory {
                    categoryName: "VOICE CHANNELS"
                    expanded: true
                    channels: [
                        {
                            name: "General",
                            type: "voice",
                            users: 3
                        },
                        {
                            name: "Gaming",
                            type: "voice",
                            users: 0
                        },
                        {
                            name: "Music",
                            type: "voice",
                            users: 1
                        }
                    ]
                    onChannelClicked: function (index) {
                        console.log("Voice channel clicked:", index);
                    }
                }

                ChannelCategory {
                    categoryName: "ARCHIVED"
                    expanded: false
                    channels: [
                        {
                            name: "old-general",
                            type: "text",
                            unread: false,
                            mentions: 0
                        },
                        {
                            name: "project-alpha",
                            type: "text",
                            unread: false,
                            mentions: 0
                        }
                    ]
                }
            }
        }

        Rectangle {
            id: userPanel
            width: parent.width
            height: 52
            color: "#292b2f"

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                Rectangle {
                    width: 32
                    height: 32
                    radius: 16
                    color: "#7289da"

                    Text {
                        anchors.centerIn: parent
                        text: "U"
                        color: "#ffffff"
                        font.pixelSize: 14
                        font.weight: Font.Bold
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 0

                    Text {
                        text: "Username"
                        color: "#ffffff"
                        font.pixelSize: 14
                        font.weight: Font.Medium
                    }

                    Text {
                        text: "#1234"
                        color: "#b9bbbe"
                        font.pixelSize: 12
                    }
                }
            }

            Row {
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                spacing: 0

                IconButton {
                    iconName: "mic"
                    size: 32
                    onClicked: console.log("Microphone toggled")
                }

                IconButton {
                    iconName: "headphones"
                    size: 32
                    onClicked: console.log("Headphones toggled")
                }

                IconButton {
                    iconName: "settings"
                    size: 32
                    onClicked: console.log("Settings opened")
                }
            }
        }
    }
}

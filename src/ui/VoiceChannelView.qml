import QtQuick
import QtQuick.Layouts

Rectangle {
    id: voiceChannelView

    property string channelName: "General"
    property string serverName: "My Server"
    property bool isConnected: true
    property bool isMuted: false
    property bool isDeafened: false
    property bool isScreenSharing: false
    property bool isCameraOn: false

    signal closeView
    signal toggleMute
    signal toggleDeafen
    signal toggleCamera
    signal toggleScreenShare
    signal leaveChannel
    signal inviteUsers
    signal openSettings

    color: "#36393f"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: headerBar
            Layout.fillWidth: true
            height: 48
            color: "#2f3136"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                IconButton {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    iconName: "chevron-right"
                    onClicked: voiceChannelView.closeView()
                }

                SvgIcon {
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    iconName: "volume"
                    iconColor: "#3ba55d"
                }

                Column {
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        text: voiceChannelView.channelName
                        color: "#ffffff"
                        font.pixelSize: 16
                        font.bold: true
                    }

                    Text {
                        text: voiceChannelView.serverName
                        color: "#72767d"
                        font.pixelSize: 12
                    }
                }

                IconButton {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    iconName: "add"
                    onClicked: voiceChannelView.inviteUsers()
                }

                IconButton {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    iconName: "settings"
                    onClicked: voiceChannelView.openSettings()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#36393f"

            GridLayout {
                id: participantsGrid
                anchors.fill: parent
                anchors.margins: 20
                columns: Math.max(1, Math.floor(width / 200))
                rowSpacing: 16
                columnSpacing: 16

                Repeater {
                    model: ListModel {
                        ListElement {
                            userId: "user1"
                            userName: "Alice"
                            avatarColor: "#5865f2"
                            isSpeaking: true
                            isMuted: false
                            isDeafened: false
                            isCameraOn: false
                            isScreenSharing: false
                        }
                        ListElement {
                            userId: "user2"
                            userName: "Bob"
                            avatarColor: "#57f287"
                            isSpeaking: false
                            isMuted: true
                            isDeafened: false
                            isCameraOn: true
                            isScreenSharing: false
                        }
                        ListElement {
                            userId: "user3"
                            userName: "Charlie"
                            avatarColor: "#feb72b"
                            isSpeaking: false
                            isMuted: false
                            isDeafened: true
                            isCameraOn: false
                            isScreenSharing: true
                        }
                        ListElement {
                            userId: "current"
                            userName: "You"
                            avatarColor: "#ed4245"
                            isSpeaking: false
                            isMuted: false
                            isDeafened: false
                            isCameraOn: false
                            isScreenSharing: false
                        }
                    }

                    delegate: VoiceParticipant {
                        Layout.preferredWidth: 180
                        Layout.preferredHeight: 180
                        Layout.alignment: Qt.AlignCenter

                        userId: model.userId
                        userName: model.userName
                        avatarColor: model.avatarColor
                        isSpeaking: model.isSpeaking
                        isMuted: model.isMuted
                        isDeafened: model.isDeafened
                        isCameraOn: model.isCameraOn
                        isScreenSharing: model.isScreenSharing
                        isCurrentUser: model.userId === "current"
                    }
                }
            }
        }

        Rectangle {
            id: controlBar
            Layout.fillWidth: true
            height: 80
            color: "#2f3136"

            RowLayout {
                anchors.centerIn: parent
                spacing: 16

                Rectangle {
                    width: 48
                    height: 48
                    radius: 24
                    color: voiceChannelView.isMuted ? "#ed4245" : "#3ba55d"

                    SvgIcon {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        iconName: "mic"
                        iconColor: "#ffffff"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: voiceChannelView.toggleMute()
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: parent.radius
                        color: "transparent"
                        border.color: "#ffffff"
                        border.width: parent.parent.containsMouse ? 2 : 0
                        opacity: 0.3
                    }
                }

                Rectangle {
                    width: 48
                    height: 48
                    radius: 24
                    color: voiceChannelView.isDeafened ? "#ed4245" : "#4f545c"

                    SvgIcon {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        iconName: "headphones"
                        iconColor: "#ffffff"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: voiceChannelView.toggleDeafen()
                    }
                }

                Rectangle {
                    width: 48
                    height: 48
                    radius: 24
                    color: voiceChannelView.isCameraOn ? "#5865f2" : "#4f545c"

                    SvgIcon {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        iconName: "settings"
                        iconColor: "#ffffff"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: voiceChannelView.toggleCamera()
                    }
                }

                Rectangle {
                    width: 48
                    height: 48
                    radius: 24
                    color: voiceChannelView.isScreenSharing ? "#5865f2" : "#4f545c"

                    SvgIcon {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        iconName: "search"
                        iconColor: "#ffffff"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: voiceChannelView.toggleScreenShare()
                    }
                }

                Rectangle {
                    width: 1
                    height: 32
                    color: "#40444b"
                }

                Rectangle {
                    width: 48
                    height: 48
                    radius: 24
                    color: "#ed4245"

                    SvgIcon {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        iconName: "bell"
                        iconColor: "#ffffff"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: voiceChannelView.leaveChannel()
                    }
                }
            }
        }
    }
}

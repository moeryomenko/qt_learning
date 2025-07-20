import QtQuick
import QtQuick.Layouts

Rectangle {
    id: voiceParticipant

    property string userId: ""
    property string userName: "User"
    property string avatarColor: "#5865f2"
    property bool isSpeaking: false
    property bool isMuted: false
    property bool isDeafened: false
    property bool isCameraOn: false
    property bool isScreenSharing: false
    property bool isCurrentUser: false

    signal clicked(string userId)

    width: 180
    height: 180
    color: "#2f3136"
    radius: 8
    border.color: isSpeaking ? "#3ba55d" : "#40444b"
    border.width: isSpeaking ? 3 : 1

    SequentialAnimation on border.width {
        running: isSpeaking
        loops: Animation.Infinite
        NumberAnimation {
            from: 2
            to: 4
            duration: 300
        }
        NumberAnimation {
            from: 4
            to: 2
            duration: 300
        }
    }

    Rectangle {
        id: videoArea
        anchors.fill: parent
        anchors.margins: 4
        radius: 6
        color: "#36393f"

        Rectangle {
            id: avatar
            anchors.centerIn: parent
            width: isCameraOn ? 0 : 80
            height: isCameraOn ? 0 : 80
            radius: width / 2
            color: voiceParticipant.avatarColor
            visible: !isCameraOn

            Behavior on width {
                NumberAnimation {
                    duration: 200
                }
            }
            Behavior on height {
                NumberAnimation {
                    duration: 200
                }
            }

            Text {
                anchors.centerIn: parent
                text: userName.charAt(0).toUpperCase()
                color: "#ffffff"
                font.pixelSize: 32
                font.bold: true
            }
        }

        Rectangle {
            anchors.fill: parent
            color: "#000000"
            radius: 6
            visible: isCameraOn

            Text {
                anchors.centerIn: parent
                text: "📹 Camera Feed"
                color: "#ffffff"
                font.pixelSize: 14
            }
        }

        Rectangle {
            anchors.fill: parent
            color: "#5865f2"
            radius: 6
            opacity: 0.8
            visible: isScreenSharing

            Text {
                anchors.centerIn: parent
                text: "🖥️ Screen Share"
                color: "#ffffff"
                font.pixelSize: 14
                font.bold: true
            }
        }

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 4
            height: 40
            color: "#000000"
            opacity: 0.8
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                Text {
                    text: userName
                    color: isSpeaking ? "#3ba55d" : "#ffffff"
                    font.pixelSize: 13
                    font.bold: isCurrentUser || isSpeaking
                    Layout.fillWidth: true
                    elide: Text.ElideRight

                    ColorAnimation on color {
                        running: isSpeaking
                        from: "#ffffff"
                        to: "#3ba55d"
                        duration: 200
                    }
                }

                Row {
                    spacing: 4

                    Rectangle {
                        width: 20
                        height: 20
                        radius: 3
                        color: "#ed4245"
                        visible: isMuted

                        SvgIcon {
                            anchors.centerIn: parent
                            width: 12
                            height: 12
                            iconName: "mic"
                            iconColor: "#ffffff"
                        }
                    }

                    Rectangle {
                        width: 20
                        height: 20
                        radius: 3
                        color: "#ed4245"
                        visible: isDeafened

                        SvgIcon {
                            anchors.centerIn: parent
                            width: 12
                            height: 12
                            iconName: "headphones"
                            iconColor: "#ffffff"
                        }
                    }

                    Rectangle {
                        width: 20
                        height: 20
                        radius: 3
                        color: "#5865f2"
                        visible: isCameraOn

                        SvgIcon {
                            anchors.centerIn: parent
                            width: 12
                            height: 12
                            iconName: "settings"
                            iconColor: "#ffffff"
                        }
                    }
                }
            }
        }

        Rectangle {
            id: speakingIndicator
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 8
            width: 8
            height: 8
            radius: 4
            color: "#3ba55d"
            visible: isSpeaking

            SequentialAnimation on opacity {
                running: isSpeaking
                loops: Animation.Infinite
                NumberAnimation {
                    from: 0.3
                    to: 1.0
                    duration: 300
                }
                NumberAnimation {
                    from: 1.0
                    to: 0.3
                    duration: 300
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor

        onClicked: voiceParticipant.clicked(voiceParticipant.userId)

        onEntered: {
            parent.scale = 1.05;
        }

        onExited: {
            parent.scale = 1.0;
        }
    }

    Behavior on scale {
        NumberAnimation {
            duration: 150
            easing.type: Easing.OutQuad
        }
    }
}

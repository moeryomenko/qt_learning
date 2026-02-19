import QtQuick
import QtQuick.Layouts
import pages

Rectangle {
    id: voiceChannelView

    property string channelName: "General"
    property string serverName: "My Server"
    property string roomId: ""

    signal leaveChannel

    color: "#36393f"

    // -----------------------------------------------------------------------
    // Auto-connect local VideoTile when RTC pipeline starts
    // -----------------------------------------------------------------------
    Connections {
        target: rtcManager

        function onRunningChanged() {
            console.log("[VoiceChannelView] rtcManager.running:", rtcManager.running)
            if (rtcManager.running) {
                rtcManager.connectLocalVideoTile(localTile)
            }
        }
    }

    // -----------------------------------------------------------------------
    // Layout
    // -----------------------------------------------------------------------
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header
        Rectangle {
            Layout.fillWidth: true
            height: 48
            color: "#2f3136"

            RowLayout {
                anchors { fill: parent; margins: 12 }
                spacing: 12

                IconButton {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    iconName: "chevron-right"
                    onClicked: {
                        console.log("[VoiceChannelView] Leave clicked — room:", voiceChannelView.roomId)
                        signalingClient.leave()
                        rtcManager.stop()
                        voiceChannelView.leaveChannel()
                    }
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

                // Peer count badge
                Rectangle {
                    visible: signalingClient.peers.length > 0
                    width: 28; height: 20; radius: 10
                    color: "#5865f2"
                    Text {
                        anchors.centerIn: parent
                        text: signalingClient.peers.length
                        color: "#ffffff"
                        font.pixelSize: 11
                        font.bold: true
                    }
                }
            }
        }

        // "Start media" banner — shown when pipeline is not running
        Rectangle {
            Layout.fillWidth: true
            height: 48
            color: "#2d3035"
            visible: !rtcManager.running

            RowLayout {
                anchors { fill: parent; leftMargin: 16; rightMargin: 16 }
                spacing: 12

                Text {
                    Layout.fillWidth: true
                    text: "Start your camera or screen share to participate"
                    color: "#b9bbbe"
                    font.pixelSize: 13
                }

                Rectangle {
                    width: 110; height: 32; radius: 4
                    color: camStartMouse.containsMouse ? "#4752c4" : "#5865f2"

                    Text { anchors.centerIn: parent; text: "Start Camera"; color: "#ffffff"; font.pixelSize: 13 }

                    MouseArea {
                        id: camStartMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            console.log("[VoiceChannelView] Start camera")
                            rtcManager.startWithCamera()
                        }
                    }
                }

                Rectangle {
                    width: 120; height: 32; radius: 4
                    color: screenStartMouse.containsMouse ? "#4f545c" : "#40444b"

                    Text { anchors.centerIn: parent; text: "Share Screen"; color: "#dcddde"; font.pixelSize: 13 }

                    MouseArea {
                        id: screenStartMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            console.log("[VoiceChannelView] Start screen share via QScreenCapture")
                            rtcManager.startScreenShare()
                        }
                    }
                }
            }
        }

        // -----------------------------------------------------------------------
        // Video tile grid
        // -----------------------------------------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            GridLayout {
                anchors { fill: parent; margins: 16 }
                columns: {
                    const total = 1 + signalingClient.peers.length
                    if (total <= 1) return 1
                    if (total <= 4) return 2
                    return 3
                }
                rowSpacing: 12
                columnSpacing: 12

                // Local video tile
                VideoTile {
                    id: localTile
                    peerId: "local"
                    displayName: apiClient.username + " (You)"
                    muted: rtcManager.micMuted
                    videoEnabled: !rtcManager.cameraMuted && rtcManager.running
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumWidth: 160
                    Layout.minimumHeight: 120

                    Component.onCompleted: {
                        console.log("[VoiceChannelView] localTile created, running:", rtcManager.running)
                        if (rtcManager.running) rtcManager.connectLocalVideoTile(localTile)
                    }
                }

                // Remote peer tiles
                Repeater {
                    model: signalingClient.peers

                    VideoTile {
                        id: peerTile
                        required property string modelData
                        peerId: modelData
                        displayName: signalingClient.peerName(modelData)
                        videoEnabled: true
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: 160
                        Layout.minimumHeight: 120

                        function connectSink() {
                            if (rtcManager.running) {
                                console.log("[VoiceChannelView] Connecting sink for peer:", peerId)
                                rtcManager.connectVideoTile(peerId, peerTile)
                            }
                        }

                        Component.onCompleted: {
                            console.log("[VoiceChannelView] peerTile created for:", modelData)
                            connectSink()
                        }

                        Connections {
                            target: rtcManager
                            function onRunningChanged() {
                                if (rtcManager.running) peerTile.connectSink()
                            }
                            function onPeerCountChanged() {
                                peerTile.connectSink()
                            }
                        }
                    }
                }
            }
        }

        // -----------------------------------------------------------------------
        // Control bar
        // -----------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            height: 80
            color: "#2f3136"

            RowLayout {
                anchors.centerIn: parent
                spacing: 16

                // Mic mute toggle
                Rectangle {
                    width: 48; height: 48; radius: 24
                    color: rtcManager.micMuted ? "#ed4245" : "#3ba55d"

                    SvgIcon {
                        anchors.centerIn: parent
                        width: 24; height: 24
                        iconName: "mic"
                        iconColor: "#ffffff"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            console.log("[VoiceChannelView] Toggle mic mute, was:", rtcManager.micMuted)
                            rtcManager.setMicMuted(!rtcManager.micMuted)
                        }
                    }
                }

                // Camera toggle
                Rectangle {
                    width: 48; height: 48; radius: 24
                    color: rtcManager.cameraMuted ? "#ed4245" : "#5865f2"

                    SvgIcon {
                        anchors.centerIn: parent
                        width: 24; height: 24
                        iconName: "settings"
                        iconColor: "#ffffff"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            console.log("[VoiceChannelView] Toggle camera mute, was:", rtcManager.cameraMuted)
                            rtcManager.setCameraMuted(!rtcManager.cameraMuted)
                        }
                    }
                }

                // Screen share
                Rectangle {
                    width: 48; height: 48; radius: 24
                    color: (rtcManager.videoSource === 1) ? "#5865f2" : "#4f545c"  // 1 = ScreenShare

                    SvgIcon {
                        anchors.centerIn: parent
                        width: 24; height: 24
                        iconName: "search"
                        iconColor: "#ffffff"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            console.log("[VoiceChannelView] Screen share clicked")
                            rtcManager.startScreenShare()
                        }
                    }
                }

                Rectangle { width: 1; height: 32; color: "#40444b" }

                // Leave channel
                Rectangle {
                    width: 48; height: 48; radius: 24
                    color: leaveMouse.containsMouse ? "#c03537" : "#ed4245"

                    SvgIcon {
                        anchors.centerIn: parent
                        width: 24; height: 24
                        iconName: "bell"
                        iconColor: "#ffffff"
                    }

                    MouseArea {
                        id: leaveMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            console.log("[VoiceChannelView] Leave channel")
                            signalingClient.leave()
                            rtcManager.stop()
                            voiceChannelView.leaveChannel()
                        }
                    }
                }
            }
        }
    }
}

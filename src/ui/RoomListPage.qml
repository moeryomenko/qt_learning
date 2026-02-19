import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property string username: apiClient.username

    signal joinRoom(string roomId, string roomName)
    signal logout

    color: "#202225"

    Component.onCompleted: {
        console.log("[RoomListPage] Loaded. user:", username)
        apiClient.listRooms()
    }

    // -----------------------------------------------------------------------
    // apiClient signals
    // -----------------------------------------------------------------------
    Connections {
        target: apiClient

        function onRoomsReceived(rooms) {
            console.log("[RoomListPage] Received", rooms.length, "rooms")
            roomModel.clear()
            for (var i = 0; i < rooms.length; ++i) {
                roomModel.append({ roomId: rooms[i].id, roomName: rooms[i].name })
            }
        }

        function onRoomCreated(room) {
            console.log("[RoomListPage] Room created:", room.name)
            createOverlay.visible = false
            createInput.text = ""
            apiClient.listRooms()
        }

        function onApiError(message) {
            console.warn("[RoomListPage] apiError:", message)
        }
    }

    ListModel { id: roomModel }

    // -----------------------------------------------------------------------
    // Layout
    // -----------------------------------------------------------------------
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header
        Rectangle {
            Layout.fillWidth: true
            height: 56
            color: "#2f3136"

            RowLayout {
                anchors { fill: parent; leftMargin: 16; rightMargin: 16 }
                spacing: 12

                Text {
                    text: "Rooms"
                    color: "#ffffff"
                    font.pixelSize: 18
                    font.bold: true
                    Layout.fillWidth: true
                }

                Text {
                    text: root.username
                    color: "#b9bbbe"
                    font.pixelSize: 13
                }

                // Logout button
                Rectangle {
                    width: 32; height: 32; radius: 16
                    color: logoutMouse.containsMouse ? "#c03537" : "#ed4245"

                    Text {
                        anchors.centerIn: parent
                        text: "↩"
                        color: "#ffffff"
                        font.pixelSize: 15
                    }

                    MouseArea {
                        id: logoutMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            console.log("[RoomListPage] Logout clicked")
                            apiClient.logout()
                            root.logout()
                        }
                    }
                }
            }
        }

        // Server URL + Refresh + New Room toolbar
        Rectangle {
            Layout.fillWidth: true
            height: 48
            color: "#36393f"

            RowLayout {
                anchors { fill: parent; leftMargin: 16; rightMargin: 16 }
                spacing: 8

                Text {
                    text: "Server:"
                    color: "#72767d"
                    font.pixelSize: 13
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 32
                    color: "#202225"
                    radius: 4

                    TextInput {
                        id: serverInput
                        anchors { fill: parent; leftMargin: 8; rightMargin: 8; topMargin: 6; bottomMargin: 6 }
                        text: apiClient.baseUrl
                        color: "#dcddde"
                        font.pixelSize: 13
                        clip: true
                        Keys.onReturnPressed: {
                            console.log("[RoomListPage] Server URL updated:", text)
                            apiClient.baseUrl = text
                            apiClient.listRooms()
                        }
                    }
                }

                Rectangle {
                    width: 72; height: 32; radius: 4
                    color: refreshMouse.containsMouse ? "#4f545c" : "#40444b"

                    Text { anchors.centerIn: parent; text: "Refresh"; color: "#dcddde"; font.pixelSize: 12 }

                    MouseArea {
                        id: refreshMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            console.log("[RoomListPage] Refresh rooms")
                            apiClient.listRooms()
                        }
                    }
                }

                Rectangle {
                    width: 90; height: 32; radius: 4
                    color: newRoomMouse.containsMouse ? "#4752c4" : "#5865f2"

                    Text { anchors.centerIn: parent; text: "+ New Room"; color: "#ffffff"; font.pixelSize: 12 }

                    MouseArea {
                        id: newRoomMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            console.log("[RoomListPage] Open create-room dialog")
                            createOverlay.visible = true
                            createInput.forceActiveFocus()
                        }
                    }
                }
            }
        }

        // Room list
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Text {
                anchors.centerIn: parent
                visible: roomModel.count === 0
                text: "No rooms yet. Create one!"
                color: "#72767d"
                font.pixelSize: 15
            }

            Flickable {
                anchors.fill: parent
                contentHeight: roomCol.implicitHeight
                clip: true

                ColumnLayout {
                    id: roomCol
                    width: parent.width
                    spacing: 1

                    Repeater {
                        model: roomModel

                        Rectangle {
                            Layout.fillWidth: true
                            height: 60
                            color: rowMouse.containsMouse ? "#3f4349" : "#2f3136"

                            RowLayout {
                                anchors { fill: parent; leftMargin: 16; rightMargin: 16 }
                                spacing: 14

                                // Room icon
                                Rectangle {
                                    width: 36; height: 36; radius: 18
                                    color: "#5865f2"
                                    Text {
                                        anchors.centerIn: parent
                                        text: model.roomName.length > 0
                                              ? model.roomName[0].toUpperCase() : "#"
                                        color: "#ffffff"
                                        font.pixelSize: 16
                                        font.bold: true
                                    }
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: model.roomName
                                    color: "#dcddde"
                                    font.pixelSize: 15
                                    elide: Text.ElideRight
                                }

                                Rectangle {
                                    width: 56; height: 30; radius: 4
                                    color: joinMouse.containsMouse ? "#3ba55d" : "#2d7d46"

                                    Text { anchors.centerIn: parent; text: "Join"; color: "#ffffff"; font.pixelSize: 13; font.bold: true }

                                    MouseArea {
                                        id: joinMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            console.log("[RoomListPage] Joining room:",
                                                        model.roomId, model.roomName)
                                            signalingClient.join(model.roomId, root.username)
                                            root.joinRoom(model.roomId, model.roomName)
                                        }
                                    }
                                }
                            }

                            MouseArea {
                                id: rowMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                z: -1
                            }
                        }
                    }
                }
            }
        }
    }

    // -----------------------------------------------------------------------
    // Create-room overlay
    // -----------------------------------------------------------------------
    Rectangle {
        id: createOverlay
        anchors.fill: parent
        color: "#80000000"
        visible: false

        MouseArea { anchors.fill: parent; onClicked: createOverlay.visible = false }

        Rectangle {
            anchors.centerIn: parent
            width: 380; height: 168
            color: "#36393f"
            radius: 8

            // Prevent clicks falling through to backdrop close
            MouseArea { anchors.fill: parent }

            ColumnLayout {
                anchors { fill: parent; margins: 24 }
                spacing: 14

                Text { text: "Create a Room"; color: "#ffffff"; font.pixelSize: 17; font.bold: true }

                Rectangle {
                    Layout.fillWidth: true
                    height: 40
                    color: "#202225"
                    radius: 4

                    TextInput {
                        id: createInput
                        anchors { fill: parent; leftMargin: 10; rightMargin: 10; topMargin: 8; bottomMargin: 8 }
                        color: "#dcddde"
                        font.pixelSize: 14
                        clip: true
                        Keys.onReturnPressed: createConfirmBtn.doCreate()
                        Keys.onEscapePressed: createOverlay.visible = false
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        width: 80; height: 34; radius: 4
                        color: cancelMouse2.containsMouse ? "#4f545c" : "#40444b"
                        Text { anchors.centerIn: parent; text: "Cancel"; color: "#dcddde"; font.pixelSize: 13 }
                        MouseArea {
                            id: cancelMouse2
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: createOverlay.visible = false
                        }
                    }

                    Rectangle {
                        id: createConfirmBtn
                        width: 80; height: 34; radius: 4
                        color: createMouse2.containsMouse ? "#4752c4" : "#5865f2"

                        function doCreate() {
                            const name = createInput.text.trim()
                            if (name === "") return
                            console.log("[RoomListPage] Creating room:", name)
                            apiClient.createRoom(name)
                        }

                        Text { anchors.centerIn: parent; text: "Create"; color: "#ffffff"; font.pixelSize: 13; font.bold: true }

                        MouseArea {
                            id: createMouse2
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: createConfirmBtn.doCreate()
                        }
                    }
                }
            }
        }
    }
}

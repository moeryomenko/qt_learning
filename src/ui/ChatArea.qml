import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    color: "#36393f"

    property string currentChannel: "general"

    signal memberListToggled

    Column {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: chatHeader
            width: parent.width
            height: 48
            color: "#36393f"

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                SvgIcon {
                    iconName: "hash"
                    iconColor: "#8e9297"
                    size: 20
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: currentChannel
                    color: "#ffffff"
                    font.pixelSize: 16
                    font.weight: Font.Bold
                    anchors.verticalCenter: parent.verticalCenter
                }

                Rectangle {
                    width: 1
                    height: 20
                    color: "#4f545c"
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: "Channel description or topic goes here"
                    color: "#8e9297"
                    font.pixelSize: 14
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Row {
                anchors.right: parent.right
                anchors.rightMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                spacing: 16

                IconButton {
                    iconName: "thread"
                    size: 24
                    onClicked: console.log("Threads clicked")
                }

                IconButton {
                    iconName: "bell"
                    size: 24
                    onClicked: console.log("Notifications clicked")
                }

                IconButton {
                    iconName: "pin"
                    size: 24
                    onClicked: console.log("Pinned messages clicked")
                }

                IconButton {
                    iconName: "users"
                    size: 24
                    onClicked: memberListToggled()
                }

                IconButton {
                    iconName: "search"
                    size: 24
                    onClicked: console.log("Search clicked")
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
            id: messagesScrollView
            width: parent.width
            height: parent.height - chatHeader.height - messageInput.height
            clip: true

            ListView {
                id: messagesList
                model: messagesModel
                spacing: 0

                delegate: MessageBubble {
                    width: messagesList.width
                    username: model.username
                    timestamp: model.timestamp
                    content: model.content
                    avatar: model.avatar
                    isGrouped: model.isGrouped
                }
            }
        }

        Rectangle {
            id: messageInput
            width: parent.width
            height: 68
            color: "#36393f"

            Rectangle {
                anchors.fill: parent
                anchors.margins: 16
                radius: 8
                color: "#40444b"

                Row {
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 12

                    IconButton {
                        iconName: "attachment"
                        size: 24
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: console.log("Attach file clicked")
                    }

                    ScrollView {
                        width: messageInput.width - 120
                        height: 24
                        anchors.verticalCenter: parent.verticalCenter

                        TextEdit {
                            id: textInput
                            width: parent.width
                            color: "#dcddde"
                            font.pixelSize: 14
                            wrapMode: TextEdit.Wrap
                            selectByMouse: true

                            Text {
                                visible: textInput.text === ""
                                text: "Message #" + currentChannel
                                color: "#72767d"
                                font: textInput.font
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                }

                Row {
                    anchors.right: parent.right
                    anchors.rightMargin: 16
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    IconButton {
                        iconName: "gift"
                        size: 24
                        onClicked: console.log("Gift clicked")
                    }

                    IconButton {
                        iconName: "gif"
                        size: 24
                        onClicked: console.log("GIF clicked")
                    }

                    IconButton {
                        iconName: "emoji"
                        size: 24
                        onClicked: console.log("Emoji clicked")
                    }
                }
            }
        }
    }

    ListModel {
        id: messagesModel

        ListElement {
            username: "Alice"
            timestamp: "Today at 2:15 PM"
            content: "Hey everyone! How's the project going?"
            avatar: "#ff6b6b"
            isGrouped: false
        }

        ListElement {
            username: "Bob"
            timestamp: "Today at 2:16 PM"
            content: "Going well! Just finished the authentication module."
            avatar: "#4ecdc4"
            isGrouped: false
        }

        ListElement {
            username: "Bob"
            timestamp: "Today at 2:16 PM"
            content: "Should be ready for testing by tomorrow."
            avatar: "#4ecdc4"
            isGrouped: true
        }

        ListElement {
            username: "Charlie"
            timestamp: "Today at 2:20 PM"
            content: "Awesome! I'll start working on the UI components then."
            avatar: "#45b7d1"
            isGrouped: false
        }

        ListElement {
            username: "Alice"
            timestamp: "Today at 2:25 PM"
            content: "Perfect. Let's have a quick standup tomorrow morning to sync up."
            avatar: "#ff6b6b"
            isGrouped: false
        }
    }
}

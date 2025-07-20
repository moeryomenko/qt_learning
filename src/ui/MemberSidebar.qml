import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    color: "#2f3136"

    Column {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: memberHeader
            width: parent.width
            height: 24
            color: "transparent"

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                text: "MEMBERS — " + (onlineModel.count + offlineModel.count)
                color: "#8e9297"
                font.pixelSize: 12
                font.weight: Font.Bold
            }
        }

        ScrollView {
            width: parent.width
            height: parent.height - memberHeader.height
            clip: true

            Column {
                width: parent.width
                spacing: 0

                MemberSection {
                    sectionTitle: "ONLINE — " + onlineModel.count
                    members: onlineModel
                }

                MemberSection {
                    sectionTitle: "OFFLINE — " + offlineModel.count
                    members: offlineModel
                    collapsed: true
                }
            }
        }
    }

    ListModel {
        id: onlineModel

        ListElement {
            username: "Alice"
            discriminator: "#1234"
            status: "online"
            activity: "Playing Valorant"
            avatar: "#ff6b6b"
            role: "Admin"
            roleColor: "#f04747"
        }

        ListElement {
            username: "Bob"
            discriminator: "#5678"
            status: "idle"
            activity: "Away"
            avatar: "#4ecdc4"
            role: "Developer"
            roleColor: "#43b581"
        }

        ListElement {
            username: "Charlie"
            discriminator: "#9012"
            status: "dnd"
            activity: "In a meeting"
            avatar: "#45b7d1"
            role: "Designer"
            roleColor: "#7289da"
        }
    }

    ListModel {
        id: offlineModel

        ListElement {
            username: "David"
            discriminator: "#3456"
            status: "offline"
            activity: ""
            avatar: "#f9ca24"
            role: "Member"
            roleColor: "#99aab5"
        }

        ListElement {
            username: "Eve"
            discriminator: "#7890"
            status: "offline"
            activity: ""
            avatar: "#6c5ce7"
            role: "Member"
            roleColor: "#99aab5"
        }
    }
}

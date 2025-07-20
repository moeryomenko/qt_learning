import QtQuick

Column {
    id: root
    width: parent.width
    spacing: 0

    property string sectionTitle: ""
    property var members
    property bool collapsed: false

    Rectangle {
        id: sectionHeader
        width: parent.width
        height: 24
        color: "transparent"

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            text: sectionTitle
            color: "#8e9297"
            font.pixelSize: 12
            font.weight: Font.Bold
        }

        MouseArea {
            anchors.fill: parent
            onClicked: collapsed = !collapsed
        }
    }

    Column {
        width: parent.width
        visible: !collapsed
        spacing: 1

        Repeater {
            model: members

            MemberItem {
                width: parent.width
                username: model.username
                discriminator: model.discriminator
                status: model.status
                activity: model.activity
                avatar: model.avatar
                role: model.role
                roleColor: model.roleColor
            }
        }
    }
}

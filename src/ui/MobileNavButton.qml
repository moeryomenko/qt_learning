import QtQuick

Rectangle {
    id: root
    height: 44
    radius: 6
    color: isActive ? "#5865f2" : (hovered ? "#4f545c" : "transparent")

    property string icon: ""
    property string iconName: ""
    property string text: ""
    property bool isActive: false
    property bool hovered: mouseArea.containsMouse

    signal clicked

    Column {
        anchors.centerIn: parent
        spacing: 2

        SvgIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            iconName: root.iconName
            size: 18
            iconColor: isActive ? "#ffffff" : "#b9bbbe"
            visible: root.iconName !== ""
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: icon
            font.pixelSize: 18
            color: isActive ? "#ffffff" : "#b9bbbe"
            visible: root.iconName === "" && icon !== ""
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.text
            font.pixelSize: 10
            color: isActive ? "#ffffff" : "#b9bbbe"
            font.weight: isActive ? Font.Medium : Font.Normal
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
            duration: 200
            easing.type: Easing.OutCubic
        }
    }
}

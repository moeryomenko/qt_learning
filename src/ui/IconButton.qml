import QtQuick

Rectangle {
    id: root
    width: size
    height: size
    radius: 4
    color: hovered ? "#4f545c" : "transparent"

    property string icon: ""
    property string iconName: ""
    property int size: 24
    property bool hovered: mouseArea.containsMouse

    signal clicked

    SvgIcon {
        anchors.centerIn: parent
        iconName: root.iconName
        size: root.size * 0.6
        iconColor: hovered ? "#dcddde" : "#b9bbbe"
        visible: root.iconName !== ""
    }

    Text {
        anchors.centerIn: parent
        text: icon
        font.pixelSize: size * 0.6
        color: hovered ? "#dcddde" : "#b9bbbe"
        visible: root.iconName === "" && icon !== ""
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
            duration: 150
            easing.type: Easing.OutCubic
        }
    }
}

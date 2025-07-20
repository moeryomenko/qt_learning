import QtQuick

Rectangle {
    id: root

    property string iconText: ""
    property string iconName: ""
    property color backgroundColor: "#36393f"
    property bool isSelected: false
    property bool isAddButton: false
    property bool hovered: mouseArea.containsMouse

    signal clicked

    color: backgroundColor
    radius: isSelected ? 16 : (hovered ? 16 : 24)

    Behavior on radius {
        NumberAnimation {
            duration: 200
            easing.type: Easing.OutCubic
        }
    }

    Rectangle {
        id: selectionIndicator
        width: 4
        height: isSelected ? parent.height * 0.8 : (hovered ? parent.height * 0.4 : 0)
        x: -8
        anchors.verticalCenter: parent.verticalCenter
        color: "#ffffff"
        radius: 2
        visible: isSelected || hovered

        Behavior on height {
            NumberAnimation {
                duration: 200
                easing.type: Easing.OutCubic
            }
        }
    }

    SvgIcon {
        anchors.centerIn: parent
        iconName: root.iconName
        iconColor: isAddButton ? "#b9bbbe" : "#ffffff"
        size: isAddButton ? 20 : 20
        visible: root.iconName !== ""
    }

    Text {
        anchors.centerIn: parent
        text: iconText
        color: isAddButton ? "#b9bbbe" : "#ffffff"
        font.pixelSize: isAddButton ? 24 : 16
        font.weight: Font.Medium
        visible: root.iconName === "" && iconText !== ""
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    states: [
        State {
            when: hovered && !isSelected
            PropertyChanges {
                target: root
                color: isAddButton ? "#3ba55c" : Qt.lighter(backgroundColor, 1.2)
            }
        }
    ]

    transitions: [
        Transition {
            ColorAnimation {
                duration: 200
                easing.type: Easing.OutCubic
            }
        }
    ]
}

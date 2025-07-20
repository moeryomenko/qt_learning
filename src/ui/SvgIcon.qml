import QtQuick

Image {
    id: root

    property string iconName: ""
    property color iconColor: "#ffffff"
    property int size: 24

    width: size
    height: size
    sourceSize.width: size
    sourceSize.height: size

    source: iconName ? "qrc:/icons/" + iconName + ".svg" : ""
}

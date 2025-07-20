import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#202225"

    property int currentTab: 0

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 0

        MobileNavButton {
            Layout.fillWidth: true
            iconName: "home"
            text: "Home"
            isActive: currentTab === 0
            onClicked: currentTab = 0
        }

        MobileNavButton {
            Layout.fillWidth: true
            iconName: "friends"
            text: "Friends"
            isActive: currentTab === 1
            onClicked: currentTab = 1
        }

        MobileNavButton {
            Layout.fillWidth: true
            iconName: "chat"
            text: "Chat"
            isActive: currentTab === 2
            onClicked: currentTab = 2
        }

        MobileNavButton {
            Layout.fillWidth: true
            iconName: "settings"
            text: "Settings"
            isActive: currentTab === 3
            onClicked: currentTab = 3
        }
    }
}

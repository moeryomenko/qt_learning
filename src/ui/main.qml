import QtQuick
import QtQuick.Window
import QtQuick.Layouts

Window {
    id: window

    readonly property int tabletBreakpoint: 768
    readonly property int desktopBreakpoint: 1024
    readonly property bool isMobile: width < tabletBreakpoint
    readonly property bool isTablet: width >= tabletBreakpoint && width < desktopBreakpoint
    readonly property bool isDesktop: width >= desktopBreakpoint

    property bool memberSidebarVisible: isDesktop

    visible: true
    width: 1200
    height: 800
    color: "#36393f"

    RowLayout {
        id: mainLayout
        anchors.fill: parent
        spacing: 0

        ServerSidebar {
            id: serverSidebar
            Layout.preferredWidth: isMobile ? 0 : 72
            Layout.fillHeight: true
            visible: !isMobile
        }

        ChannelSidebar {
            id: channelSidebar
            Layout.preferredWidth: isMobile ? 0 : 240
            Layout.fillHeight: true
            visible: !isMobile
        }

        ChatArea {
            id: chatArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            onMemberListToggled: memberSidebarVisible = !memberSidebarVisible
        }

        MemberSidebar {
            id: memberSidebar
            Layout.preferredWidth: memberSidebarVisible ? 240 : 0
            Layout.fillHeight: true
            visible: memberSidebarVisible && !isMobile
        }
    }

    MobileNavigation {
        id: mobileNav
        anchors.bottom: parent.bottom
        width: parent.width
        height: 60
        visible: isMobile
    }
}

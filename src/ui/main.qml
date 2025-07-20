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
    property bool voiceChannelViewVisible: false
    property string currentVoiceChannel: ""
    property string currentVoiceServer: ""

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
            visible: !isMobile && !voiceChannelViewVisible

            onVoiceChannelClicked: function (channelName, serverName) {
                currentVoiceChannel = channelName;
                currentVoiceServer = serverName;
                voiceChannelViewVisible = true;
            }
        }

        ChatArea {
            id: chatArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !voiceChannelViewVisible
            onMemberListToggled: memberSidebarVisible = !memberSidebarVisible
        }

        VoiceChannelView {
            id: voiceChannelView
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: voiceChannelViewVisible
            channelName: currentVoiceChannel
            serverName: currentVoiceServer

            onCloseView: {
                voiceChannelViewVisible = false;
            }

            onToggleMute: {
                console.log("Toggle mute");
            }

            onToggleDeafen: {
                console.log("Toggle deafen");
            }

            onToggleCamera: {
                console.log("Toggle camera");
            }

            onToggleScreenShare: {
                console.log("Toggle screen share");
            }

            onLeaveChannel: {
                voiceChannelViewVisible = false;
                console.log("Left voice channel");
            }

            onInviteUsers: {
                console.log("Invite users");
            }

            onOpenSettings: {
                console.log("Open settings");
            }
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

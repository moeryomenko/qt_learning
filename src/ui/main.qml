import QtQuick
import QtQuick.Window
import QtQuick.Layouts

Window {
    id: window

    // -----------------------------------------------------------------------
    // Navigation state: 0 = Login, 1 = Room List, 2 = Voice Channel
    // -----------------------------------------------------------------------
    property int appState: 0
    property string currentRoomId: ""
    property string currentRoomName: ""

    // Responsive helpers (kept for compatibility with existing sub-components)
    readonly property int tabletBreakpoint: 768
    readonly property int desktopBreakpoint: 1024
    readonly property bool isMobile: width < tabletBreakpoint
    readonly property bool isDesktop: width >= desktopBreakpoint

    visible: true
    width: 1200
    height: 800
    title: "WebRTC Chat"
    color: "#36393f"

    // -----------------------------------------------------------------------
    // Login page (state 0)
    // -----------------------------------------------------------------------
    LoginPage {
        anchors.fill: parent
        visible: appState === 0
        z: appState === 0 ? 1 : -1

        onLoggedIn: function(username) {
            console.log("[main.qml] Logged in as:", username)
            appState = 1
        }
    }

    // -----------------------------------------------------------------------
    // Room list (state 1)
    // -----------------------------------------------------------------------
    RoomListPage {
        anchors.fill: parent
        visible: appState === 1
        z: appState === 1 ? 1 : -1

        onJoinRoom: function(roomId, roomName) {
            console.log("[main.qml] Join room:", roomId, roomName)
            window.currentRoomId = roomId
            window.currentRoomName = roomName
            appState = 2
        }

        onLogout: {
            console.log("[main.qml] Logout")
            appState = 0
        }
    }

    // -----------------------------------------------------------------------
    // Voice / video channel view (state 2)
    // -----------------------------------------------------------------------
    VoiceChannelView {
        anchors.fill: parent
        visible: appState === 2
        z: appState === 2 ? 1 : -1

        channelName: window.currentRoomName
        serverName: "WebRTC Chat"
        roomId: window.currentRoomId

        onLeaveChannel: {
            console.log("[main.qml] Left channel, returning to room list")
            appState = 1
        }
    }
}

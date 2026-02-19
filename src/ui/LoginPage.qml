import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property int currentTab: 0      // 0 = Sign In, 1 = Register
    property string statusText: ""
    property bool statusIsError: false
    property bool busy: false

    signal loggedIn(string username)

    color: "#202225"

    // -----------------------------------------------------------------------
    // Wire to apiClient context property
    // -----------------------------------------------------------------------
    Connections {
        target: apiClient

        function onLoginSuccess(tokens) {
            console.log("[LoginPage] loginSuccess — user:", apiClient.username)
            root.statusText = "Login successful!"
            root.statusIsError = false
            root.busy = false
            root.loggedIn(apiClient.username)
        }

        function onLoginError(message) {
            console.warn("[LoginPage] loginError:", message)
            root.statusText = "Sign-in failed: " + message
            root.statusIsError = true
            root.busy = false
        }

        function onRegisterSuccess(tokens) {
            console.log("[LoginPage] registerSuccess — user:", apiClient.username)
            root.statusText = "Account created! You are now signed in."
            root.statusIsError = false
            root.busy = false
            root.loggedIn(apiClient.username)
        }

        function onRegisterError(message) {
            console.warn("[LoginPage] registerError:", message)
            root.statusText = "Registration failed: " + message
            root.statusIsError = true
            root.busy = false
        }
    }

    // -----------------------------------------------------------------------
    // Centered card
    // -----------------------------------------------------------------------
    Rectangle {
        id: card
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.9, 460)
        height: cardColumn.implicitHeight + 48
        color: "#36393f"
        radius: 8

        ColumnLayout {
            id: cardColumn
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                margins: 32
                topMargin: 32
            }
            spacing: 0

            // Title
            Text {
                Layout.fillWidth: true
                text: "Welcome to WebRTC Chat"
                color: "#ffffff"
                font.pixelSize: 22
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }

            Item { Layout.preferredHeight: 6 }

            Text {
                Layout.fillWidth: true
                text: "Sign in to join a room or create a new account."
                color: "#b9bbbe"
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            Item { Layout.preferredHeight: 20 }

            // Tab bar
            RowLayout {
                Layout.fillWidth: true
                spacing: 0

                Repeater {
                    model: ["Sign In", "Register"]

                    Rectangle {
                        Layout.fillWidth: true
                        height: 38
                        color: "transparent"

                        // Active underline
                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 2
                            color: root.currentTab === index ? "#5865f2" : "transparent"
                        }

                        Text {
                            anchors.centerIn: parent
                            text: modelData
                            color: root.currentTab === index ? "#ffffff" : "#72767d"
                            font.pixelSize: 13
                            font.bold: root.currentTab === index
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                console.log("[LoginPage] Tab switched to:", index)
                                root.currentTab = index
                                root.statusText = ""
                            }
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 18 }

            // ---------------------------------------------------------------
            // Sign-in form
            // ---------------------------------------------------------------
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 10
                visible: root.currentTab === 0

                Text {
                    text: "USERNAME"
                    color: "#8e9297"
                    font.pixelSize: 11
                    font.bold: true
                    font.letterSpacing: 0.5
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 40
                    color: "#202225"
                    radius: 3

                    TextInput {
                        id: loginUsername
                        anchors { fill: parent; leftMargin: 10; rightMargin: 10; topMargin: 8; bottomMargin: 8 }
                        color: "#dcddde"
                        font.pixelSize: 14
                        clip: true
                        enabled: !root.busy
                        Keys.onReturnPressed: loginBtn.doLogin()
                        onTextChanged: root.statusText = ""
                    }
                }

                Text {
                    text: "PASSWORD"
                    color: "#8e9297"
                    font.pixelSize: 11
                    font.bold: true
                    font.letterSpacing: 0.5
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 40
                    color: "#202225"
                    radius: 3

                    TextInput {
                        id: loginPassword
                        anchors { fill: parent; leftMargin: 10; rightMargin: 10; topMargin: 8; bottomMargin: 8 }
                        color: "#dcddde"
                        font.pixelSize: 14
                        echoMode: TextInput.Password
                        clip: true
                        enabled: !root.busy
                        Keys.onReturnPressed: loginBtn.doLogin()
                        onTextChanged: root.statusText = ""
                    }
                }

                Item { height: 4 }

                Rectangle {
                    id: loginBtn
                    Layout.fillWidth: true
                    height: 44
                    radius: 3
                    color: loginMouse.containsMouse && !root.busy ? "#4752c4" : "#5865f2"
                    opacity: root.busy ? 0.7 : 1.0

                    function doLogin() {
                        if (root.busy) return
                        const user = loginUsername.text.trim()
                        if (user === "" || loginPassword.text === "") {
                            root.statusText = "Please enter username and password."
                            root.statusIsError = true
                            return
                        }
                        console.log("[LoginPage] login attempt:", user)
                        root.busy = true
                        root.statusText = "Signing in…"
                        root.statusIsError = false
                        apiClient.login(user, loginPassword.text)
                    }

                    Text {
                        anchors.centerIn: parent
                        text: root.busy ? "Signing in…" : "Sign In"
                        color: "#ffffff"
                        font.pixelSize: 14
                        font.bold: true
                    }

                    MouseArea {
                        id: loginMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        enabled: !root.busy
                        onClicked: loginBtn.doLogin()
                    }
                }
            }

            // ---------------------------------------------------------------
            // Register form
            // ---------------------------------------------------------------
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 10
                visible: root.currentTab === 1

                Text {
                    text: "USERNAME"
                    color: "#8e9297"
                    font.pixelSize: 11
                    font.bold: true
                    font.letterSpacing: 0.5
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 40
                    color: "#202225"
                    radius: 3
                    TextInput {
                        id: regUsername
                        anchors { fill: parent; leftMargin: 10; rightMargin: 10; topMargin: 8; bottomMargin: 8 }
                        color: "#dcddde"
                        font.pixelSize: 14
                        clip: true
                        enabled: !root.busy
                        onTextChanged: root.statusText = ""
                    }
                }

                Text {
                    text: "PASSWORD"
                    color: "#8e9297"
                    font.pixelSize: 11
                    font.bold: true
                    font.letterSpacing: 0.5
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 40
                    color: "#202225"
                    radius: 3
                    TextInput {
                        id: regPassword
                        anchors { fill: parent; leftMargin: 10; rightMargin: 10; topMargin: 8; bottomMargin: 8 }
                        color: "#dcddde"
                        font.pixelSize: 14
                        echoMode: TextInput.Password
                        clip: true
                        enabled: !root.busy
                        onTextChanged: root.statusText = ""
                    }
                }

                Text {
                    text: "CONFIRM PASSWORD"
                    color: "#8e9297"
                    font.pixelSize: 11
                    font.bold: true
                    font.letterSpacing: 0.5
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 40
                    color: "#202225"
                    radius: 3
                    TextInput {
                        id: regConfirm
                        anchors { fill: parent; leftMargin: 10; rightMargin: 10; topMargin: 8; bottomMargin: 8 }
                        color: "#dcddde"
                        font.pixelSize: 14
                        echoMode: TextInput.Password
                        clip: true
                        enabled: !root.busy
                        Keys.onReturnPressed: regBtn.doRegister()
                        onTextChanged: root.statusText = ""
                    }
                }

                Item { height: 4 }

                Rectangle {
                    id: regBtn
                    Layout.fillWidth: true
                    height: 44
                    radius: 3
                    color: regMouse.containsMouse && !root.busy ? "#4752c4" : "#5865f2"
                    opacity: root.busy ? 0.7 : 1.0

                    function doRegister() {
                        if (root.busy) return
                        const user = regUsername.text.trim()
                        const pass = regPassword.text
                        if (user === "" || pass === "") {
                            root.statusText = "Please fill in all fields."
                            root.statusIsError = true
                            return
                        }
                        if (pass !== regConfirm.text) {
                            root.statusText = "Passwords do not match."
                            root.statusIsError = true
                            return
                        }
                        console.log("[LoginPage] register attempt:", user)
                        root.busy = true
                        root.statusText = "Creating account…"
                        root.statusIsError = false
                        apiClient.registerUser(user, pass)
                    }

                    Text {
                        anchors.centerIn: parent
                        text: root.busy ? "Creating account…" : "Create Account"
                        color: "#ffffff"
                        font.pixelSize: 14
                        font.bold: true
                    }

                    MouseArea {
                        id: regMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        enabled: !root.busy
                        onClicked: regBtn.doRegister()
                    }
                }
            }

            // ---------------------------------------------------------------
            // Status line
            // ---------------------------------------------------------------
            Text {
                Layout.fillWidth: true
                Layout.topMargin: 12
                text: root.statusText
                color: root.statusIsError ? "#ed4245" : "#3ba55d"
                font.pixelSize: 13
                wrapMode: Text.WordWrap
                visible: root.statusText !== ""
                horizontalAlignment: Text.AlignHCenter
            }

            Item { Layout.preferredHeight: 16 }
        }
    }
}

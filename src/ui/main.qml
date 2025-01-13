import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts

Window {
    id: window

    readonly property int responsiveWidth: 500

    visible: true
    width: 300
    height: 500

    SwipeView {
        id: swipeView

        currentIndex: 1
        anchors.fill: parent
        states: [
            State {
                when: window.width >= window.responsiveWidth

                ParentChange {
                    target: contacts
                    parent: contactsContainer
                }

                ParentChange {
                    target: chat
                    parent: chatContainer
                }

                PropertyChanges {
                    indicator: {
                        visible: hide;
                    }
                }
            }
        ]

        Item {
            Rectangle {
                id: contacts

                anchors.fill: parent
                color: "lightblue"

                border {
                    width: 5
                    color: "white"
                }
            }
        }

        Item {
            Rectangle {
                id: chat

                anchors.fill: parent
                color: "lightgray"

                border {
                    width: 5
                    color: "white"
                }
            }
        }
    }

    PageIndicator {
        id: indicator

        count: swipeView.count
        currentIndex: swipeView.currentIndex

        anchors {
            bottom: swipeView.bottom
            bottomMargin: 60
            horizontalCenter: swipeView.horizontalCenter
        }
    }

    Row {
        id: splitView

        anchors.fill: parent

        Item {
            id: contactsContainer

            width: parent.width / 2
            height: parent.height
        }

        Item {
            id: chatContainer

            width: parent.width / 2
            height: parent.height
        }
    }

    Rectangle {
        id: bottomPanel

        anchors.bottom: parent.bottom
        width: parent.width
        height: 50
        color: "darkgray"

        RowLayout {
            anchors.fill: parent
            spacing: 10

            Button {
                text: "Button 1"
                Layout.fillWidth: true
            }

            Button {
                text: "Button 2"
                Layout.fillWidth: true
            }

            Button {
                text: "Button 3"
                Layout.fillWidth: true
            }
        }
    }
}

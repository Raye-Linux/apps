// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

// What a card shows in the tests instead of the live picture: a small
// drawn desktop for a screen, a drawn window for a window. Same
// properties as LiveThumbnail.qml.
import QtQuick

Item {
    id: preview

    property string kind
    property string outputName
    property string uuid
    property int number
    property string title
    property var iconSource
    readonly property bool live: true

    // a different light for each screen and window
    readonly property real seed: ((number * 37 + title.length * 11) % 100) / 100

    Rectangle {
        anchors.fill: parent
        radius: 4
        visible: preview.kind === "screen"
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0; color: Qt.hsla(0.74 + preview.seed * 0.06, 0.55, 0.16, 1) }
            GradientStop { position: 1; color: Qt.hsla(0.80, 0.6, 0.08, 1) }
        }

        // the glow of the wallpaper
        Rectangle {
            x: parent.width * (0.2 + preview.seed * 0.4)
            y: parent.height * 0.15
            width: parent.width * 0.5
            height: width
            radius: width / 2
            color: "#c040ff"
            opacity: 0.18
        }
        // two windows
        Rectangle {
            x: parent.width * 0.12; y: parent.height * 0.18
            width: parent.width * 0.46; height: parent.height * 0.5
            radius: 2; color: "#221b33"; border.color: "#33ffffff"
            Rectangle { width: parent.width; height: 5; radius: 2; color: "#2d2541" }
        }
        Rectangle {
            x: parent.width * 0.44; y: parent.height * 0.32
            width: parent.width * 0.42; height: parent.height * 0.44
            radius: 2; color: "#f6f3fb"; opacity: 0.92
            Rectangle { width: parent.width; height: 5; radius: 2; color: "#e7e3ec" }
            Column {
                x: 6; y: 10; spacing: 4
                Repeater {
                    model: 4
                    Rectangle { width: (index % 2 ? 0.5 : 0.7) * 60; height: 2; color: "#b4afbc" }
                }
            }
        }
        // the taskbar
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width; height: 8
            color: "#d9140f20"
            Row {
                anchors.centerIn: parent
                spacing: 3
                Repeater {
                    model: 5
                    Rectangle { width: 4; height: 4; radius: 1; color: index === 0 ? "#7b3cff" : "#b9a8d6" }
                }
            }
        }
    }

    // a window: its frame and a page of lines
    Rectangle {
        anchors.centerIn: parent
        width: parent.width * 0.86
        height: parent.height * 0.92
        visible: preview.kind === "window"
        radius: 3
        color: preview.seed > 0.5 ? "#f6f3fb" : "#1b1529"
        border.color: "#33ffffff"
        Rectangle {
            width: parent.width; height: 10; radius: 3
            color: preview.seed > 0.5 ? "#e7e3ec" : "#2a2338"
        }
        Column {
            x: 10; y: 18; spacing: 6
            Repeater {
                model: 6
                Rectangle {
                    width: (0.4 + ((index * 13 + preview.number * 7) % 10) / 20) * 150
                    height: 3
                    radius: 1.5
                    color: index === 0 ? "#7b3cff" : (preview.seed > 0.5 ? "#c9c2d6" : "#4a405f")
                }
            }
        }
    }
}

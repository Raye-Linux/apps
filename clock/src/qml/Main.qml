// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The clock's window: the navigation pane on the left (Timer, Alarm,
// Stopwatch, World clock) and one page at a time, sliding in softly.
//
// Every colour, size and duration comes from Tokens.

import QtCore
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Window
import org.raye.ui
import org.raye.clock

MicaWindow {
    id: root

    width: 1100
    height: 740
    minimumWidth: 560
    minimumHeight: 520
    title: i18n("Clock")
    visible: true

    readonly property var pages: [
        { glyph: "timer", text: i18nc("@title:tab", "Timer") },
        { glyph: "alarm", text: i18nc("@title:tab", "Alarm") },
        { glyph: "stopwatch", text: i18nc("@title:tab", "Stopwatch") },
        { glyph: "globe", text: i18nc("@title:tab", "World clock") },
    ]
    property int page: Math.max(0, Math.min(prefs.page, pages.length - 1))
    readonly property bool shown: root.visibility !== Window.Minimized && root.visibility !== Window.Hidden
    // the pane folds to its icons in a narrow window, as in Windows
    readonly property bool wide: width >= 900
    property bool paneOpen: wide && prefs.paneOpen

    // the time of day the pages draw
    property date now: new Date()
    Timer {
        interval: 1000
        running: root.shown
        repeat: true
        triggeredOnStart: true
        onTriggered: root.now = new Date()
    }

    // called by main.cpp for --timers, --alarms, --stopwatch, --world
    function showPage(index) {
        prefs.page = index
    }

    Settings {
        id: prefs
        category: "Window"
        property int page: 1
        property bool paneOpen: true
    }

    ClockModel {
        id: clockModel
    }

    Shortcut { sequence: "Ctrl+1"; onActivated: prefs.page = 0 }
    Shortcut { sequence: "Ctrl+2"; onActivated: prefs.page = 1 }
    Shortcut { sequence: "Ctrl+3"; onActivated: prefs.page = 2 }
    Shortcut { sequence: "Ctrl+4"; onActivated: prefs.page = 3 }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // --- the navigation pane --------------------------------------------
        ColumnLayout {
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.preferredWidth: root.paneOpen ? 280 : 56
            Layout.topMargin: Tokens.sizeTitlebar
            Layout.leftMargin: Tokens.s2
            Layout.rightMargin: Tokens.s2
            spacing: Tokens.s1
            Behavior on Layout.preferredWidth {
                NumberAnimation { duration: Tokens.durationNormal; easing.type: Tokens.easingNormal }
            }

            NavButton {
                Layout.preferredWidth: 40
                Layout.bottomMargin: Tokens.s3
                glyph: "menu"
                text: root.paneOpen ? i18nc("@info:tooltip", "Close navigation") : i18nc("@info:tooltip", "Open navigation")
                expanded: false
                enabled: root.wide
                opacity: enabled ? 1 : 0.4
                onClicked: prefs.paneOpen = !prefs.paneOpen
            }
            Repeater {
                model: root.pages
                delegate: NavButton {
                    required property var modelData
                    required property int index
                    Layout.fillWidth: true
                    glyph: modelData.glyph
                    text: modelData.text
                    expanded: root.paneOpen
                    selected: root.page === index
                    onClicked: prefs.page = index
                }
            }
            Item { Layout.fillHeight: true }
        }

        // --- the page ----------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: Tokens.sizeTitlebar
            color: Qt.alpha(Tokens.bgLayerSolid, root.mica ? 0.6 : 1)
            radius: Tokens.radiusWindow
            border.width: 1
            border.color: Tokens.strokeCard
            // only the top-left corner rounds into the pane, as in Windows' Mica layers
            Rectangle {
                anchors.right: parent.right
                width: parent.radius
                height: parent.height
                color: parent.color
            }
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: parent.radius
                color: parent.color
            }

            Repeater {
                model: [timers, alarms, stopwatch, world]
                delegate: Loader {
                    id: holder
                    required property var modelData
                    required property int index
                    readonly property bool current: root.page === index
                    anchors.fill: parent
                    // a page is made the first time it is shown, then kept
                    active: current || item !== null
                    sourceComponent: modelData
                    visible: opacity > 0
                    opacity: current ? 1 : 0
                    // in from a little below, as Windows' page transition
                    property real shift: current ? 0 : 24
                    transform: Translate { y: holder.shift }
                    Behavior on opacity {
                        NumberAnimation { duration: holder.current ? Tokens.durationSlow : Tokens.durationExitShort }
                    }
                    Behavior on shift {
                        NumberAnimation { duration: holder.current ? Tokens.durationSlow : 0; easing.type: Tokens.easingSlow }
                    }
                }
            }
        }
    }

    Component {
        id: timers
        TimersPage {
            clock: clockModel
            shown: root.shown && root.page === 0
        }
    }
    Component {
        id: alarms
        AlarmsPage {
            clock: clockModel
        }
    }
    Component {
        id: stopwatch
        StopwatchPage {
            clock: clockModel
            shown: root.shown && root.page === 2
        }
    }
    Component {
        id: world
        WorldPage {
            clock: clockModel
            now: root.now
        }
    }
}

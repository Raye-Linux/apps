// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The Task Manager's window: the search box along the top, the
// navigation pane on the left, and one page at a time.
//
// App history is not one of the pages: Linux keeps no per-app total
// across sessions, and an empty page where Windows shows a month of
// figures reads as a fault in the machine. See ../README.md.
//
// Every colour, radius, spacing and duration comes from Tokens, so the
// window follows the user's accent and the Night/Day scheme with no
// colour of its own.

import QtCore
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Window
import org.raye.ui
import org.raye.taskmanager

MicaWindow {
    id: root

    width: 1100
    height: 720
    minimumWidth: 720
    minimumHeight: 480
    title: i18n("Task Manager")
    visible: true

    readonly property var pages: [
        { glyph: "processes", text: i18nc("@title:tab", "Processes") },
        { glyph: "performance", text: i18nc("@title:tab", "Performance") },
        { glyph: "startup", text: i18nc("@title:tab", "Startup apps") },
        { glyph: "users", text: i18nc("@title:tab", "Users") },
        { glyph: "details", text: i18nc("@title:tab", "Details") },
        { glyph: "services", text: i18nc("@title:tab", "Services") },
    ]
    readonly property int settingsPage: pages.length
    property int page: Math.max(0, Math.min(prefs.startPage, pages.length))
    readonly property bool shown: root.visibility !== Window.Minimized && root.visibility !== Window.Hidden
    // the window's update speed: 1, 2 or 4 seconds, or none
    readonly property int interval: prefs.speed === "high" ? 500 : prefs.speed === "low" ? 4000 : 1000

    Settings {
        id: prefs
        category: "TaskManager"
        property int startPage: 0
        property string speed: "normal"
        property bool navOpen: true
    }

    ProcessModel {
        id: processes
        active: root.shown && prefs.speed !== "paused"
        intervalMs: root.interval
        listed: root.page === 4
    }

    PerformanceModel {
        id: performance
        active: root.shown && prefs.speed !== "paused"
    }

    Shortcut {
        sequences: [StandardKey.Find]
        onActivated: searchBox.forceActiveFocus()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // the search box, centred as in Windows 11
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: Tokens.sizeTitlebar + Tokens.s4

            SearchField {
                id: searchBox
                anchors.centerIn: parent
                width: Math.min(420, parent.width - 2 * Tokens.s8)
                visible: root.page !== 1 && root.page !== 3 && root.page !== root.settingsPage
                placeholderText: i18nc("@info:placeholder", "Type a name or a process number to search")
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // the navigation pane
            ColumnLayout {
                Layout.fillHeight: true
                Layout.fillWidth: false
                Layout.preferredWidth: prefs.navOpen ? 252 : 48 + Tokens.s2
                Layout.leftMargin: Tokens.s1
                Layout.bottomMargin: Tokens.s3
                spacing: Tokens.s1

                Behavior on Layout.preferredWidth {
                    NumberAnimation { duration: Tokens.durationNormal; easing.type: Tokens.easingNormal }
                }

                NavButton {
                    Layout.preferredWidth: 40
                    glyph: "menu"
                    expanded: false
                    text: prefs.navOpen ? i18nc("@action:button", "Close navigation") : i18nc("@action:button", "Open navigation")
                    onClicked: prefs.navOpen = !prefs.navOpen
                }

                Repeater {
                    model: root.pages
                    delegate: NavButton {
                        required property var modelData
                        required property int index
                        Layout.fillWidth: true
                        Layout.rightMargin: Tokens.s2
                        glyph: modelData.glyph
                        text: modelData.text
                        expanded: prefs.navOpen
                        selected: root.page === index
                        onClicked: root.page = index
                    }
                }

                Item { Layout.fillHeight: true }

                NavButton {
                    Layout.fillWidth: true
                    Layout.rightMargin: Tokens.s2
                    glyph: "settings"
                    text: i18nc("@title:tab", "Settings")
                    expanded: prefs.navOpen
                    selected: root.page === root.settingsPage
                    onClicked: root.page = root.settingsPage
                }
            }

            // the page, on its layer
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                topLeftRadius: Tokens.radiusOverlay
                color: Tokens.bgLayer
                border.width: 1
                border.color: Tokens.strokeCard

                StackLayout {
                    anchors.fill: parent
                    anchors.margins: Tokens.s6
                    anchors.rightMargin: Tokens.s5
                    currentIndex: root.page

                    ProcessesPage {
                        model: processes
                        performance: performance
                        search: root.page === 0 ? searchBox.text : ""
                        onRunTaskRequested: runSheet.open()
                        onDetailsRequested: (pid) => { details.reveal(pid); root.page = 4 }
                    }
                    PerformancePage {
                        performance: performance
                        processCount: processes.totalCount
                        onRunTaskRequested: runSheet.open()
                    }
                    StartupPage {
                        active: root.page === 2 && root.shown
                        search: root.page === 2 ? searchBox.text : ""
                    }
                    UsersPage {
                        model: processes
                    }
                    DetailsPage {
                        id: details
                        model: processes
                        search: root.page === 4 ? searchBox.text : ""
                        onRunTaskRequested: runSheet.open()
                    }
                    ServicesPage {
                        active: root.page === 5 && root.shown
                        search: root.page === 5 ? searchBox.text : ""
                    }
                    SettingsPage {
                        pages: root.pages
                        startPage: prefs.startPage
                        speed: prefs.speed
                        onStartPageChosen: (index) => prefs.startPage = index
                        onSpeedChosen: (value) => prefs.speed = value
                    }
                }
            }
        }
    }

    RunTaskSheet {
        id: runSheet
        parent: root.contentItem
        model: processes
    }
}

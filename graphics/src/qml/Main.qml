// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The graphics card panel's window: the navigation pane on the left
// (Overview, and Performance and Fan on an AMD card) and one page at a
// time. The changes of Performance and Fan wait in one set until Apply.
//
// Every colour, size and duration comes from Tokens.

pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Window
import org.raye.ui
import org.raye.graphics

MicaWindow {
    id: root

    width: 1040
    height: 720
    minimumWidth: 640
    minimumHeight: 480
    title: i18n("Graphics Card")
    visible: true

    readonly property bool tunable: graphics.card.tunable === true
    readonly property var pages: tunable ? [
        { glyph: "overview", text: i18nc("@title:tab", "Overview") },
        { glyph: "tune", text: i18nc("@title:tab", "Performance") },
        { glyph: "fan", text: i18nc("@title:tab", "Fan") },
    ] : [
        { glyph: "overview", text: i18nc("@title:tab", "Overview") },
    ]
    property int page: Math.max(0, Math.min(prefs.page, pages.length - 1))
    readonly property bool shown: root.visibility !== Window.Minimized && root.visibility !== Window.Hidden
    readonly property bool wide: width >= 860
    property bool paneOpen: wide && prefs.paneOpen

    // the settings being edited, the same for the Performance and Fan pages
    property var pending: graphics.saved
    property string message
    property bool messageOk: true

    function plain(s) {
        return JSON.stringify({ performance: s.performance, profile: s.profile, powerCapW: s.powerCapW,
                                fanCurve: s.fanCurve, curve: s.fanCurve ? s.curve : [] })
    }
    readonly property bool dirty: plain(pending) !== plain(graphics.saved)

    // called by main.cpp for --performance and --fan
    function showPage(index) {
        prefs.page = index
    }

    Settings {
        id: prefs
        category: "Window"
        property int page: 0
        property bool paneOpen: true
    }

    GraphicsModel {
        id: graphics
        active: root.shown
        onLimitsChanged: root.pending = graphics.saved
        onFinished: (ok, text) => {
            root.messageOk = ok
            root.message = text
        }
        onCardChanged: root.message = ""
    }

    Shortcut { sequence: "Ctrl+1"; onActivated: prefs.page = 0 }
    Shortcut { sequence: "Ctrl+2"; onActivated: prefs.page = 1 }
    Shortcut { sequence: "Ctrl+3"; onActivated: prefs.page = 2 }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // --- the navigation pane --------------------------------------------
        ColumnLayout {
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.preferredWidth: root.paneOpen ? 260 : 56
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
                expanded: false
                text: root.paneOpen ? i18nc("@info:tooltip", "Close navigation") : i18nc("@info:tooltip", "Open navigation")
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
            topLeftRadius: Tokens.radiusOverlay
            color: Tokens.bgLayer
            border.width: 1
            border.color: Tokens.strokeCard

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Tokens.s7
                anchors.rightMargin: Tokens.s5
                anchors.bottomMargin: Tokens.s5
                spacing: Tokens.s4

                // more than one card: which one these pages are about
                RowLayout {
                    Layout.fillWidth: true
                    visible: graphics.cards.length > 1
                    spacing: Tokens.s4
                    Glyph {
                        kind: "card"
                        color: Tokens.textSecondary
                    }
                    QQC2.Label {
                        text: i18nc("@label", "Graphics card")
                        color: Tokens.textSecondary
                        font: Tokens.body
                    }
                    ChoiceButton {
                        implicitWidth: 320
                        choices: graphics.cards.map(c => c.model || c.slot)
                        currentIndex: graphics.current
                        enabled: !graphics.busy
                        onChosen: (index) => graphics.current = index
                    }
                    Item { Layout.fillWidth: true }
                }

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    // the pages scroll: their full height is no reason to
                    // push the action bar out of the window
                    Layout.minimumHeight: 0
                    Layout.preferredHeight: 0
                    currentIndex: root.page

                    OverviewPage {
                        model: graphics
                    }
                    TuningPage {
                        model: graphics
                        pending: root.pending
                        onChanged: (p) => root.pending = p
                    }
                    FanPage {
                        model: graphics
                        pending: root.pending
                        onChanged: (p) => root.pending = p
                    }
                }

                ActionBar {
                    Layout.fillWidth: true
                    visible: root.tunable && (root.page === 1 || (root.page === 2 && graphics.limits.canCurve === true))
                    dirty: root.dirty
                    valid: graphics.acceptable(root.pending)
                    busy: graphics.busy
                    atDefaults: false
                    message: root.message
                    messageOk: root.messageOk
                    onApplyClicked: { root.message = ""; graphics.apply(root.pending) }
                    onDiscardClicked: root.pending = graphics.saved
                    onRestoreClicked: { root.message = ""; graphics.restoreDefaults() }
                    onMessageClosed: root.message = ""
                }
            }
        }
    }
}

// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Timers, as many as you like, side by side on cards. They count down
// with the window closed too; raye-clock-tool rings at the end.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

Item {
    id: page

    property var clock
    property bool shown: true
    property bool editing: false
    // the time the cards count from: fast while a timer runs and the page shows
    property double nowMs: Date.now()
    Timer {
        interval: 50
        repeat: true
        running: page.shown && page.clock.timersRunning
        onTriggered: page.nowMs = Date.now()
        onRunningChanged: page.nowMs = Date.now()
    }

    // "5 min", "1 hr 30 min": a timer with no name is called by its length
    function lengthText(ms) {
        const total = Math.round(ms / 1000)
        const h = Math.floor(total / 3600)
        const m = Math.floor(total / 60) % 60
        const s = total % 60
        const parts = []
        if (h)
            parts.push(i18ncp("@info timer length", "%1 hr", "%1 hr", h))
        if (m)
            parts.push(i18ncp("@info timer length", "%1 min", "%1 min", m))
        if (s)
            parts.push(i18ncp("@info timer length", "%1 sec", "%1 sec", s))
        return parts.join(" ")
    }

    GridView {
        id: grid
        anchors.fill: parent
        anchors.leftMargin: Tokens.s8
        anchors.topMargin: Tokens.s8
        clip: true
        // as many 320 px columns as fit, stretched to the width
        readonly property int columns: Math.max(1, Math.floor((width - Tokens.s8 + Tokens.s5) / (320 + Tokens.s5)))
        cellWidth: Math.floor((width - Tokens.s8) / columns)
        cellHeight: 360 + Tokens.s5
        model: page.clock.timers
        boundsBehavior: Flickable.StopAtBounds
        footer: Item { width: 1; height: 96 }
        QQC2.ScrollBar.vertical: Scroller {}
        WheelScroll { target: grid }

        add: Transition {
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Tokens.durationSlow; easing.type: Tokens.easingSlow }
                NumberAnimation { property: "scale"; from: 0.92; to: 1; duration: Tokens.durationSlow; easing.type: Tokens.easingSlow }
            }
        }
        remove: Transition {
            ParallelAnimation {
                NumberAnimation { property: "opacity"; to: 0; duration: Tokens.durationExit; easing.type: Tokens.easingExit }
                NumberAnimation { property: "scale"; to: 0.92; duration: Tokens.durationExit; easing.type: Tokens.easingExit }
            }
        }
        displaced: Transition {
            NumberAnimation { properties: "x,y"; duration: Tokens.durationNormal; easing.type: Tokens.easingNormal }
        }

        delegate: Item {
            id: cell
            required property string key
            required property string name
            required property double durationMs
            required property double leftMs
            required property double endsAt
            required property bool running
            width: grid.cellWidth
            height: grid.cellHeight

            TimerCard {
                anchors.fill: parent
                anchors.rightMargin: Tokens.s5
                anchors.bottomMargin: Tokens.s5
                clock: page.clock
                title: cell.name !== "" ? cell.name : page.lengthText(cell.durationMs)
                durationMs: cell.durationMs
                remainingMs: cell.running ? Math.max(0, cell.endsAt - page.nowMs) : cell.leftMs
                endsAt: cell.endsAt
                running: cell.running
                editing: page.editing
                onToggle: cell.running ? page.clock.pauseTimer(cell.key) : page.clock.startTimer(cell.key)
                onReset: page.clock.resetTimer(cell.key)
                onRemove: page.clock.removeTimer(cell.key)
                onEdit: sheet.openEdit(cell.key, cell.name, cell.durationMs)
            }
        }
    }

    // nothing here yet
    ColumnLayout {
        anchors.centerIn: parent
        visible: page.clock.timers.count === 0
        spacing: Tokens.s4
        Glyph {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 48
            Layout.preferredHeight: 48
            kind: "timer"
            stroke: 1
            color: Tokens.textTertiary
        }
        QQC2.Label {
            Layout.alignment: Qt.AlignHCenter
            text: i18nc("@info", "No timers yet")
            color: Tokens.textPrimary
            font: Tokens.subtitle
        }
        QQC2.Label {
            Layout.alignment: Qt.AlignHCenter
            text: i18nc("@info", "Select + to add one.")
            color: Tokens.textSecondary
            font: Tokens.body
        }
    }

    // the page's buttons, bottom right, as Windows places them
    Row {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Tokens.s7
        spacing: Tokens.s3
        RoundButton {
            glyph: page.editing ? "check" : "edit"
            tip: page.editing ? i18nc("@action:button", "Done") : i18nc("@action:button", "Edit timers")
            visible: page.clock.timers.count > 0
            onClicked: page.editing = !page.editing
        }
        RoundButton {
            glyph: "add"
            accent: true
            tip: i18nc("@action:button", "Add new timer")
            onClicked: sheet.openNew()
        }
    }

    TimerSheet {
        id: sheet
        clock: page.clock
    }
}

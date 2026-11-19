// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The alarms, on cards. They ring with the window closed: the next one is
// a systemd user timer, which starts raye-clock-tool at its time.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

Item {
    id: page

    property var clock
    property bool editing: false
    property bool noticeClosed: false

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: Tokens.s8
        anchors.topMargin: Tokens.s8
        spacing: Tokens.s5

        // a sleeping PC wakes for an alarm when its clock can do it; one
        // that is shut down never does
        InfoBar {
            objectName: "wakeNotice"
            Layout.fillWidth: true
            Layout.rightMargin: Tokens.s8
            visible: !page.noticeClosed
            title: page.clock && page.clock.canWake
                   ? i18nc("@info", "Alarms wake your PC from sleep.")
                   : i18nc("@info", "This PC can't wake from sleep for alarms.")
            message: page.clock && page.clock.canWake
                     ? i18nc("@info", "They do not ring when it is shut down. An alarm missed then shows as a notification when you sign in.")
                     : i18nc("@info", "Alarms only ring while it is on. An alarm missed while it slept shows as a notification when it wakes.")
            onCloseClicked: page.noticeClosed = true
        }

        GridView {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            readonly property int columns: Math.max(1, Math.floor((width - Tokens.s8 + Tokens.s5) / (320 + Tokens.s5)))
            cellWidth: Math.floor((width - Tokens.s8) / columns)
            cellHeight: 212 + Tokens.s5
            model: page.clock.alarms
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
                required property int hour
                required property int minute
                required property string clock
                required property string meridiem
                required property int days
                required property bool enabled
                required property string sound
                required property int snooze
                required property string ringsIn
                required property string snoozed
                width: grid.cellWidth
                height: grid.cellHeight

                AlarmCard {
                    anchors.fill: parent
                    anchors.rightMargin: Tokens.s5
                    anchors.bottomMargin: Tokens.s5
                    model: page.clock
                    name: cell.name
                    clockText: cell.clock
                    meridiem: cell.meridiem
                    days: cell.days
                    on: cell.enabled
                    ringsIn: cell.snoozed !== "" ? cell.snoozed : cell.ringsIn
                    editing: page.editing
                    onSwitched: on => page.clock.setAlarmEnabled(cell.key, on)
                    onRemove: page.clock.removeAlarm(cell.key)
                    onEdit: sheet.openEdit(cell)
                }
            }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        visible: page.clock.alarms.count === 0
        spacing: Tokens.s4
        Glyph {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 48
            Layout.preferredHeight: 48
            kind: "alarm"
            stroke: 1
            color: Tokens.textTertiary
        }
        QQC2.Label {
            Layout.alignment: Qt.AlignHCenter
            text: i18nc("@info", "No alarms yet")
            color: Tokens.textPrimary
            font: Tokens.subtitle
        }
        QQC2.Label {
            Layout.alignment: Qt.AlignHCenter
            text: i18nc("@info", "Select + to set one.")
            color: Tokens.textSecondary
            font: Tokens.body
        }
    }

    Row {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Tokens.s7
        spacing: Tokens.s3
        RoundButton {
            glyph: page.editing ? "check" : "edit"
            tip: page.editing ? i18nc("@action:button", "Done") : i18nc("@action:button", "Edit alarms")
            visible: page.clock.alarms.count > 0
            onClicked: page.editing = !page.editing
        }
        RoundButton {
            glyph: "add"
            accent: true
            tip: i18nc("@action:button", "Add an alarm")
            onClicked: sheet.openNew()
        }
    }

    AlarmSheet {
        id: sheet
        clock: page.clock
    }
}

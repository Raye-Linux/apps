// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// A time of the editor: typed ("9", "9:30", "14h15", "2:30 pm") or picked
// from the half hours.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui
import "datelogic.js" as D

QQC2.TextField {
    id: field

    // minutes since midnight
    property int minutes: 9 * 60
    signal picked(int minutes)

    function label(m) {
        return D.timeText(new Date(2000, 0, 1, Math.floor(m / 60), m % 60))
    }
    function parse(text) {
        const t = text.trim().toLowerCase()
        const m = t.match(/^(\d{1,2})(?:\s*[:h.]\s*(\d{1,2}))?\s*(am|pm|a|p)?$/)
        if (!m)
            return -1
        let h = Number(m[1]), min = m[2] ? Number(m[2]) : 0
        if (m[3] && m[3].startsWith("p") && h < 12)
            h += 12
        if (m[3] && m[3].startsWith("a") && h === 12)
            h = 0
        return h < 24 && min < 60 ? h * 60 + min : -1
    }

    implicitWidth: 120
    implicitHeight: Tokens.sizeControlHeight
    text: label(minutes)
    color: Tokens.textPrimary
    selectionColor: Tokens.accent
    selectedTextColor: Tokens.textOnAccent
    font: Tokens.body
    leftPadding: Tokens.s4
    rightPadding: 28
    verticalAlignment: Text.AlignVCenter
    onEditingFinished: {
        const m = parse(text)
        if (m >= 0)
            picked(m)
        text = label(m >= 0 ? m : minutes)
    }
    onMinutesChanged: if (!activeFocus) text = label(minutes)

    background: Rectangle {
        radius: Tokens.radiusControl
        color: field.activeFocus ? Tokens.fillInputActive : field.hovered ? Tokens.fillControlHover : Tokens.fillControl
        border.width: 1
        border.color: Tokens.strokeControl
        Rectangle {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom; leftMargin: 1; rightMargin: 1 }
            height: field.activeFocus ? 2 : 1
            radius: Tokens.radiusControl
            color: field.activeFocus ? Tokens.accent : Tokens.strokeControlBottom
        }
        FlatButton {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: 28
            height: parent.height - 4
            glyph: "chevronDown"
            tip: i18nc("@info:tooltip", "Pick a time")
            onClicked: {
                times.open()
                times.list.positionViewAtIndex(Math.floor(field.minutes / 30), ListView.Center)
            }
        }
    }

    QQC2.Popup {
        id: times
        readonly property alias list: list
        y: field.height + 2
        width: field.width
        height: 240
        padding: Tokens.sizeMenuPadding
        focus: true
        background: Rectangle {
            radius: Tokens.radiusOverlay
            color: Tokens.bgTertiary
            border.color: Tokens.strokeSurface
            border.width: 1
        }
        contentItem: ListView {
            id: list
            clip: true
            model: 48
            delegate: QQC2.AbstractButton {
                id: slot
                required property int index
                width: ListView.view.width
                height: Tokens.sizeMenuItem
                hoverEnabled: true
                text: field.label(index * 30)
                onClicked: {
                    times.close()
                    field.picked(index * 30)
                }
                background: Rectangle {
                    radius: Tokens.radiusControl
                    color: slot.index * 30 === field.minutes ? Tokens.fillSubtleSelected
                         : slot.hovered ? Tokens.fillSubtleHover : "transparent"
                }
                contentItem: QQC2.Label {
                    leftPadding: Tokens.s3
                    verticalAlignment: Text.AlignVCenter
                    text: slot.text
                    color: Tokens.textPrimary
                    font: Tokens.body
                }
            }
        }
    }
}

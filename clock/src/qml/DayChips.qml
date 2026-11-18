// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The days an alarm rings, one round chip per day in the week's own order
// (Monday or Sunday first, as the language says). Bit 0 is Monday.

import QtQuick
import QtQuick.Controls as QQC2
import org.raye.ui

Row {
    id: chips

    property int days: 0
    property bool editable: true
    property real size: 32
    // the model, for the day letters
    property var clock
    signal toggled(int days)

    spacing: editable ? Tokens.s2 : Tokens.s1

    Repeater {
        model: 7
        delegate: QQC2.AbstractButton {
            id: chip
            required property int index
            // 1 Monday .. 7 Sunday, starting on the week's first day
            readonly property int day: (Qt.locale().firstDayOfWeek + 6 + index) % 7 + 1
            readonly property bool on: (chips.days & (1 << (day - 1))) !== 0

            width: chips.size
            height: chips.size
            enabled: chips.editable
            hoverEnabled: chips.editable
            checkable: chips.editable
            checked: on
            focusPolicy: chips.editable ? Qt.StrongFocus : Qt.NoFocus
            Accessible.name: chips.clock ? chips.clock.dayName(day) : ""
            Accessible.role: Accessible.CheckBox
            onClicked: chips.toggled(chips.days ^ (1 << (day - 1)))

            background: Rectangle {
                radius: width / 2
                color: chip.on ? (chips.editable && chip.hovered ? Tokens.accentHover : Tokens.accent)
                     : chips.editable ? (chip.pressed ? Tokens.fillControlPressed : chip.hovered ? Tokens.fillControlHover : Tokens.fillControl)
                     : "transparent"
                border.width: chip.on ? 0 : 1
                border.color: chips.editable ? Tokens.strokeControl : Tokens.strokeDivider
                Behavior on color { ColorAnimation { duration: Tokens.durationFast } }
                FocusRing {
                    anchors.fill: parent
                    radius: width / 2
                    visible: chip.visualFocus
                }
            }
            contentItem: Text {
                text: chips.clock ? chips.clock.dayLetter(chip.day) : ""
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: chip.on ? Tokens.textOnAccent : Tokens.textSecondary
                font.family: "Inter"
                font.pixelSize: chips.size >= 32 ? 12 : 10
                font.weight: chip.on ? Font.DemiBold : Font.Normal
            }
        }
    }
}

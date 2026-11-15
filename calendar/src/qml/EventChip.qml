// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// One event in a day of the month, or along the all-day strip: its colour
// down the left edge, its time and its title. A holiday has no colour and
// is not a button.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui
import "datelogic.js" as D

QQC2.AbstractButton {
    id: chip

    property var occ
    property bool showTime: true
    readonly property color tint: occ.color ? occ.color : Tokens.accent

    implicitHeight: 22
    hoverEnabled: !occ.holiday
    enabled: !occ.holiday
    text: occ.title !== "" ? occ.title : i18nc("@info an event with no title", "(No title)")
    Accessible.name: text

    background: Rectangle {
        radius: Tokens.radiusControl
        color: chip.occ.holiday ? "transparent"
             : chip.pressed ? Qt.alpha(chip.tint, 0.32) : chip.hovered ? Qt.alpha(chip.tint, 0.26) : Qt.alpha(chip.tint, 0.18)
        Rectangle {
            visible: !chip.occ.holiday
            width: 3
            height: parent.height - 6
            anchors.verticalCenter: parent.verticalCenter
            x: 3
            radius: 1.5
            color: chip.tint
        }
    }

    contentItem: RowLayout {
        spacing: Tokens.s2
        QQC2.Label {
            Layout.leftMargin: chip.occ.holiday ? Tokens.s2 : 10
            visible: chip.showTime && !chip.occ.allDay
            text: D.timeText(chip.occ.start)
            color: Tokens.textSecondary
            font: Tokens.caption
        }
        QQC2.Label {
            Layout.fillWidth: true
            Layout.leftMargin: (chip.showTime && !chip.occ.allDay) ? 0 : (chip.occ.holiday ? Tokens.s2 : 10)
            Layout.rightMargin: Tokens.s2
            text: chip.text
            elide: Text.ElideRight
            color: chip.occ.holiday ? Tokens.textSecondary : Tokens.textPrimary
            font: Tokens.caption
        }
    }
}

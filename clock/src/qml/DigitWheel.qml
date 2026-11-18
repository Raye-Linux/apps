// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// One column of the time picker: big digits on a wheel that turns with
// the mouse wheel, a drag, the arrow keys or a click on a number. The one
// chosen sits on the accent band in the middle.

import QtQuick
import QtQuick.Controls as QQC2
import org.raye.ui

QQC2.Tumbler {
    id: wheel

    // the texts shown, in order; value is the index chosen
    property var labels: []
    property int value: 0
    signal moved(int value)

    model: labels.length
    implicitWidth: 76
    implicitHeight: 3 * 48
    visibleItemCount: 3
    wrap: labels.length > 3
    focusPolicy: Qt.StrongFocus
    currentIndex: value
    onCurrentIndexChanged: if (currentIndex !== value) moved(currentIndex)
    onValueChanged: if (currentIndex !== value) currentIndex = value
    Accessible.role: Accessible.SpinBox

    Keys.onUpPressed: currentIndex = (currentIndex - 1 + count) % count
    Keys.onDownPressed: currentIndex = (currentIndex + 1) % count

    background: Rectangle {
        radius: Tokens.radiusOverlay
        color: wheel.hovered || wheel.activeFocus ? Tokens.fillSubtleHover : "transparent"
        Rectangle {
            anchors.centerIn: parent
            width: parent.width - Tokens.s3
            height: 44
            radius: Tokens.radiusControl
            color: Qt.alpha(Tokens.accent, wheel.activeFocus ? 1 : 0.85)
        }
        FocusRing {
            anchors.fill: parent
            radius: Tokens.radiusOverlay
            visible: wheel.visualFocus
        }
    }

    delegate: Text {
        required property int index
        readonly property real away: Math.abs(QQC2.Tumbler.displacement)
        text: wheel.labels[index] ?? ""
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: away < 0.5 ? Tokens.textOnAccent : Tokens.textSecondary
        opacity: 1 - Math.min(0.6, away * 0.45)
        font.family: "Inter Display"
        font.pixelSize: 28
        font.weight: away < 0.5 ? Font.DemiBold : Font.Normal
        font.features: { "tnum": 1 }
        TapHandler { onTapped: wheel.currentIndex = parent.index }
    }
}

// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// A check box and its words, in the window's own style.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

QQC2.AbstractButton {
    id: box

    checkable: true
    hoverEnabled: true
    implicitHeight: Tokens.sizeControlHeight
    implicitWidth: row.implicitWidth
    Accessible.role: Accessible.CheckBox
    Accessible.name: text

    contentItem: RowLayout {
        id: row
        spacing: Tokens.s3
        Rectangle {
            Layout.preferredWidth: Tokens.sizeCheckbox
            Layout.preferredHeight: Tokens.sizeCheckbox
            radius: Tokens.radiusControl
            color: box.checked ? (box.hovered ? Tokens.accentHover : Tokens.accent) : box.hovered ? Tokens.fillControlHover : Tokens.fillControl
            border.width: box.checked ? 0 : 1
            border.color: Tokens.strokeStrong
            Canvas {
                anchors.fill: parent
                visible: box.checked
                onPaint: {
                    const c = getContext("2d")
                    c.reset()
                    c.strokeStyle = Tokens.textOnAccent
                    c.lineWidth = 1.5
                    c.lineCap = "round"
                    c.beginPath()
                    c.moveTo(5, 10.5); c.lineTo(8.5, 14); c.lineTo(15, 6.5)
                    c.stroke()
                }
            }
            FocusRing {
                anchors.fill: parent
                visible: box.visualFocus
            }
        }
        QQC2.Label {
            text: box.text
            color: Tokens.textPrimary
            font: Tokens.body
        }
    }
}

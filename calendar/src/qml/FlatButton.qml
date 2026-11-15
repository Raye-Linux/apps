// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// A flat button of the command bar: a glyph, a word, or both.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

QQC2.AbstractButton {
    id: button

    property string glyph
    property string tip

    implicitHeight: Tokens.sizeControlHeight
    implicitWidth: text === "" ? Tokens.sizeControlHeight : row.implicitWidth + 2 * Tokens.s4
    hoverEnabled: true
    opacity: enabled ? 1 : 0.45
    Accessible.name: text !== "" ? text : tip

    QQC2.ToolTip.visible: tip !== "" && hovered
    QQC2.ToolTip.delay: 700
    QQC2.ToolTip.text: tip

    background: Rectangle {
        radius: Tokens.radiusControl
        color: button.pressed ? Tokens.fillSubtlePressed : button.hovered ? Tokens.fillSubtleHover : "transparent"
        FocusRing {
            anchors.fill: parent
            visible: button.visualFocus
        }
    }

    contentItem: Item {
        RowLayout {
            id: row
            anchors.centerIn: parent
            spacing: Tokens.s3
            Glyph {
                visible: button.glyph !== ""
                kind: button.glyph
                color: Tokens.textPrimary
            }
            QQC2.Label {
                visible: button.text !== ""
                text: button.text
                color: Tokens.textPrimary
                font: Tokens.body
            }
        }
    }
}

// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// A flat button of a page's command bar: a glyph and a word.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

QQC2.AbstractButton {
    id: button

    property string glyph

    implicitHeight: Tokens.sizeControlHeight
    implicitWidth: row.implicitWidth + 2 * Tokens.s4
    hoverEnabled: true
    opacity: enabled ? 1 : 0.45
    Accessible.name: text

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
                text: button.text
                color: Tokens.textPrimary
                font: Tokens.body
            }
        }
    }
}

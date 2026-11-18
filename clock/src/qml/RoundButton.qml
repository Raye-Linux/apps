// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// A round button with one glyph: the accent one starts and pauses, the
// plain ones reset, mark a lap or add. A soft press shrinks it a little.

import QtQuick
import QtQuick.Controls as QQC2
import org.raye.ui

QQC2.AbstractButton {
    id: button

    property string glyph
    property bool accent: false
    property string tip
    property real size: 40

    implicitWidth: size
    implicitHeight: size
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    Accessible.name: tip
    QQC2.ToolTip.visible: tip !== "" && hovered
    QQC2.ToolTip.delay: 700
    QQC2.ToolTip.text: tip

    scale: pressed ? 0.94 : 1
    Behavior on scale { NumberAnimation { duration: Tokens.durationFaster; easing.type: Easing.OutCubic } }

    background: Rectangle {
        radius: width / 2
        color: !button.enabled ? (button.accent ? Tokens.accentDisabled : Tokens.fillControlDisabled)
             : button.accent ? (button.pressed ? Tokens.accentPressed : button.hovered ? Tokens.accentHover : Tokens.accent)
             : button.pressed ? Tokens.fillControlPressedSolid : button.hovered ? Tokens.fillControlHoverSolid : Tokens.fillControlSolid
        border.width: button.accent ? 0 : 1
        border.color: Tokens.strokeControlSolid
        Behavior on color { ColorAnimation { duration: Tokens.durationFaster } }
        FocusRing {
            anchors.fill: parent
            radius: width / 2
            visible: button.visualFocus
        }
    }

    contentItem: Item {
        Glyph {
            anchors.centerIn: parent
            width: button.size >= 48 ? 20 : 16
            height: width
            kind: button.glyph
            color: button.accent ? Tokens.textOnAccent : button.enabled ? Tokens.textPrimary : Tokens.textDisabled
        }
    }
}

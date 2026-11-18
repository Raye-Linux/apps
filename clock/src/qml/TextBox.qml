// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// A one-line text box: control fill, and the accent line under it while typing.

import QtQuick
import QtQuick.Controls as QQC2
import org.raye.ui

QQC2.TextField {
    id: f

    implicitHeight: Tokens.sizeControlHeight
    color: Tokens.textPrimary
    placeholderTextColor: Tokens.textTertiary
    selectionColor: Tokens.accent
    selectedTextColor: Tokens.textOnAccent
    font: Tokens.body
    leftPadding: Tokens.s4
    rightPadding: Tokens.s4
    hoverEnabled: true

    background: Rectangle {
        radius: Tokens.radiusControl
        color: f.activeFocus ? Tokens.fillInputActive : f.hovered ? Tokens.fillControlHover : Tokens.fillControl
        border.width: 1
        border.color: Tokens.strokeControl
        Rectangle {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom; leftMargin: 1; rightMargin: 1 }
            height: f.activeFocus ? 2 : 1
            radius: Tokens.radiusControl
            color: f.activeFocus ? Tokens.accent : Tokens.strokeControlBottom
        }
    }
}

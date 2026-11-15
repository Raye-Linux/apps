// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// A one-line text field in the window's style.

import QtQuick
import QtQuick.Controls as QQC2
import org.raye.ui

QQC2.TextField {
    id: field

    implicitHeight: Tokens.sizeControlHeight
    leftPadding: Tokens.s4
    rightPadding: Tokens.s4
    color: enabled ? Tokens.textPrimary : Tokens.textDisabled
    placeholderTextColor: Tokens.textSecondary
    selectionColor: Tokens.accent
    selectedTextColor: Tokens.textOnAccent
    font: Tokens.body
    selectByMouse: true

    background: Rectangle {
        radius: Tokens.radiusControl
        color: !field.enabled ? Tokens.fillControlDisabled : field.activeFocus ? Tokens.fillInputActive
             : field.hovered ? Tokens.fillControlHover : Tokens.fillControl
        border.width: 1
        border.color: Tokens.strokeControl

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 1
            anchors.rightMargin: 1
            height: field.activeFocus ? 2 : 1
            radius: Tokens.radiusControl
            color: field.activeFocus ? Tokens.accent : Tokens.strokeControlBottom
        }
    }
}

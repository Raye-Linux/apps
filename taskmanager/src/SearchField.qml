// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The search box at the top of the window, as in Windows 11.

import QtQuick
import QtQuick.Controls as QQC2
import org.raye.ui

QQC2.TextField {
    id: field

    property bool showGlyph: true

    implicitHeight: Tokens.sizeControlHeight
    leftPadding: showGlyph ? Tokens.s4 + 16 + Tokens.s3 : Tokens.s4
    rightPadding: Tokens.s4
    color: Tokens.textPrimary
    placeholderTextColor: Tokens.textSecondary
    selectionColor: Tokens.accent
    selectedTextColor: Tokens.textOnAccent
    font: Tokens.body
    Accessible.name: placeholderText

    Keys.onEscapePressed: field.clear()

    background: Rectangle {
        radius: Tokens.radiusControl
        color: field.activeFocus ? Tokens.fillInputActive : field.hovered ? Tokens.fillControlHover : Tokens.fillControl
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

        Glyph {
            x: Tokens.s4
            anchors.verticalCenter: parent.verticalCenter
            visible: field.showGlyph
            kind: "search"
            color: Tokens.textSecondary
        }
    }
}

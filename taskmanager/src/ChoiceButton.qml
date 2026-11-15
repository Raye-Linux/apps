// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// A drop-down of a few choices, drawn like the other controls of the
// window and opening the same menu as a right click.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

QQC2.AbstractButton {
    id: button

    property var choices: [] // strings
    property int currentIndex: 0
    signal chosen(int index)

    implicitHeight: Tokens.sizeControlHeight
    implicitWidth: 200
    hoverEnabled: true
    text: choices[currentIndex] ?? ""
    Accessible.role: Accessible.ComboBox
    Accessible.name: text

    onClicked: menu.openAt(button, 0, button.height + Tokens.s1)

    background: Rectangle {
        radius: Tokens.radiusControl
        color: button.pressed ? Tokens.fillControlPressed : button.hovered ? Tokens.fillControlHover : Tokens.fillControl
        border.width: 1
        border.color: Tokens.strokeControl
        FocusRing {
            anchors.fill: parent
            visible: button.visualFocus
        }
    }
    contentItem: RowLayout {
        spacing: Tokens.s3
        QQC2.Label {
            Layout.fillWidth: true
            elide: Text.ElideRight
            text: button.text
            color: Tokens.textPrimary
            font: Tokens.body
        }
        Glyph {
            Layout.preferredWidth: 12
            Layout.preferredHeight: 12
            kind: "chevronDown"
            color: Tokens.textSecondary
        }
    }
    leftPadding: Tokens.s4
    rightPadding: Tokens.s4

    ContextMenu {
        id: menu
        items: button.choices.map((text, index) => ({ text: text, action: () => button.chosen(index) }))
    }
}

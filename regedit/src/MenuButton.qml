// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// One word of the menu bar; opens its menu under itself.

import QtQuick
import QtQuick.Controls as QQC2
import org.raye.ui

QQC2.AbstractButton {
    id: button

    property var items: []

    implicitHeight: Tokens.sizeControlCompact + Tokens.s2
    implicitWidth: label.implicitWidth + 2 * Tokens.s4
    hoverEnabled: true
    Accessible.role: Accessible.MenuItem
    Accessible.name: text

    onClicked: menu.openAt(button, 0, button.height + Tokens.s1)

    background: Rectangle {
        radius: Tokens.radiusControl
        color: menu.opened || button.pressed ? Tokens.fillSubtlePressed : button.hovered ? Tokens.fillSubtleHover : "transparent"
    }

    contentItem: QQC2.Label {
        id: label
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: button.text
        color: Tokens.textPrimary
        font: Tokens.body
    }

    ContextMenu {
        id: menu
        items: button.items
    }
}

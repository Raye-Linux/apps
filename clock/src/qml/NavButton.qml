// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// One entry of the navigation pane: the glyph, the label when the pane is
// open, and the accent pill on the selected one.

import QtQuick
import QtQuick.Controls as QQC2
import org.raye.ui

QQC2.AbstractButton {
    id: button

    property string glyph
    property bool selected: false
    property bool expanded: true

    implicitHeight: Tokens.sizeNavItem
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    QQC2.ToolTip.visible: !expanded && hovered && text !== ""
    QQC2.ToolTip.text: text
    QQC2.ToolTip.delay: 600
    Accessible.name: text

    background: Rectangle {
        radius: Tokens.radiusControl
        color: button.selected ? Tokens.fillSubtleSelected
             : button.pressed ? Tokens.fillSubtlePressed
             : button.hovered ? Tokens.fillSubtleHover : "transparent"
        Behavior on color { ColorAnimation { duration: Tokens.durationFaster } }

        Rectangle {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: Tokens.sizePillWidth
            height: button.selected ? Tokens.sizePillHeight : 0
            radius: Tokens.radiusNavPill
            color: Tokens.accent
            Behavior on height { NumberAnimation { duration: Tokens.durationFast; easing.type: Tokens.easingFast } }
        }

        FocusRing {
            anchors.fill: parent
            visible: button.visualFocus
        }
    }

    contentItem: Item {
        Glyph {
            id: icon
            x: 16 - button.leftPadding
            anchors.verticalCenter: parent.verticalCenter
            kind: button.glyph
            color: Tokens.textPrimary
        }
        QQC2.Label {
            anchors.left: icon.right
            anchors.leftMargin: Tokens.s5
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            visible: button.expanded
            elide: Text.ElideRight
            text: button.text
            color: Tokens.textPrimary
            font: button.selected ? Tokens.bodyStrong : Tokens.body
        }
    }
    leftPadding: 0
    rightPadding: Tokens.s3
}

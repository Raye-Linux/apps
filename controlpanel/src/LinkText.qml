// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// A link of the Control Panel, underlined under the pointer. Tasks are in
// the accent; names of items and categories are plain text.

import QtQuick
import QtQuick.Controls as QQC2
import org.raye.ui

QQC2.AbstractButton {
    id: link

    property font textFont: Tokens.body
    property bool strong: false
    property bool plain: false

    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    implicitWidth: label.implicitWidth
    implicitHeight: label.implicitHeight + Tokens.s1
    Accessible.role: Accessible.Link
    Accessible.name: text

    contentItem: QQC2.Label {
        id: label
        text: link.text
        color: link.plain ? (link.pressed ? Tokens.textSecondary : Tokens.textPrimary)
                          : (link.pressed ? Tokens.accent : Tokens.accentText)
        font.family: link.textFont.family
        font.pointSize: link.textFont.pointSize
        font.weight: link.strong ? Font.DemiBold : link.textFont.weight
        font.underline: link.hovered
        elide: Text.ElideRight
    }

    background: FocusRing {
        visible: link.visualFocus
    }
}

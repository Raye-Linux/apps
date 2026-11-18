// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// A drop-down of the editor: options is a list of { text, value }.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

QQC2.AbstractButton {
    id: box

    property var options: []
    property var value
    signal picked(var value)

    readonly property int current: {
        for (let i = 0; i < options.length; ++i)
            if (options[i].value === value)
                return i
        return -1
    }

    implicitHeight: Tokens.sizeControlHeight
    implicitWidth: 220
    hoverEnabled: true
    text: current >= 0 ? options[current].text : ""
    Accessible.role: Accessible.ComboBox
    onClicked: popup.open()

    background: Rectangle {
        radius: Tokens.radiusControl
        color: box.pressed ? Tokens.fillControlPressed : box.hovered ? Tokens.fillControlHover : Tokens.fillControl
        border.width: 1
        border.color: Tokens.strokeControl
        FocusRing {
            anchors.fill: parent
            visible: box.visualFocus
        }
    }

    contentItem: RowLayout {
        QQC2.Label {
            Layout.fillWidth: true
            Layout.leftMargin: Tokens.s4
            text: box.text
            color: Tokens.textPrimary
            font: Tokens.body
            elide: Text.ElideRight
        }
        Glyph {
            Layout.rightMargin: Tokens.s4
            kind: "chevronDown"
            color: Tokens.textSecondary
        }
    }

    MenuPopup {
        id: popup
        y: box.height + 2
        minimumWidth: box.width - 2 * padding
        items: box.options.map(o => ({ text: o.text, checked: o.value === box.value, action: () => box.picked(o.value) }))
    }
}

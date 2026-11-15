// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// System Tools: the programs for people who look after the PC,
// those that are installed.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui
import org.raye.controlpanel

ColumnLayout {
    id: page

    // the tools installed on this PC
    property var tools: []

    signal toolOpened(var tool)

    spacing: Tokens.s2

    QQC2.Label {
        Layout.bottomMargin: Tokens.s4
        text: i18nc("@title", "System Tools")
        color: Tokens.textPrimary
        font: Tokens.title
    }

    Repeater {
        model: page.tools
        delegate: QQC2.AbstractButton {
            id: tool
            required property var modelData
            Layout.fillWidth: true
            Layout.maximumWidth: 720
            implicitHeight: 64
            hoverEnabled: true
            Accessible.name: modelData.name
            onClicked: page.toolOpened(modelData)

            background: Rectangle {
                radius: Tokens.radiusOverlay
                color: tool.pressed ? Tokens.fillControlPressed : tool.hovered ? Tokens.bgCardHover : Tokens.bgCard
                border.width: 1
                border.color: Tokens.strokeCard
            }
            contentItem: RowLayout {
                spacing: Tokens.s5
                Glyph {
                    Layout.leftMargin: Tokens.s5
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    kind: tool.modelData.glyph
                    color: Tokens.textSecondary
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    QQC2.Label { text: tool.modelData.name; color: Tokens.textPrimary; font: Tokens.body }
                    QQC2.Label {
                        Layout.fillWidth: true
                        text: tool.modelData.summary
                        elide: Text.ElideRight
                        color: Tokens.textSecondary
                        font: Tokens.caption
                    }
                }
                Glyph {
                    Layout.rightMargin: Tokens.s5
                    kind: "chevron"
                    color: Tokens.textSecondary
                }
            }
        }
    }

    QQC2.Label {
        visible: page.tools.length === 0
        text: i18n("None of these tools are installed.")
        color: Tokens.textSecondary
        font: Tokens.body
    }
}

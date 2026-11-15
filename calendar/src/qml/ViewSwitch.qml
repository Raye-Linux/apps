// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Day, Week, Month, Agenda: one row of buttons, the shown one underlined.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

Rectangle {
    id: root

    property var names: []
    property int current: 0
    signal chosen(int index)

    implicitHeight: Tokens.sizeControlHeight
    implicitWidth: row.implicitWidth + 2 * Tokens.s1
    radius: Tokens.radiusControl
    color: Tokens.fillControl
    border.width: 1
    border.color: Tokens.strokeControl

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: Tokens.s1
        spacing: Tokens.s1
        Repeater {
            model: root.names
            QQC2.AbstractButton {
                id: tab
                required property int index
                required property string modelData
                readonly property bool on: index === root.current
                Layout.fillHeight: true
                implicitWidth: label.implicitWidth + 2 * Tokens.s4
                hoverEnabled: true
                text: modelData
                Accessible.role: Accessible.PageTab
                Accessible.checked: on
                onClicked: root.chosen(index)
                background: Rectangle {
                    radius: Tokens.radiusControl - 1
                    color: tab.on ? Tokens.fillSubtleSelected : tab.hovered ? Tokens.fillSubtleHover : "transparent"
                    Rectangle {
                        visible: tab.on
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        width: 16
                        height: 3
                        radius: 1.5
                        color: Tokens.accent
                    }
                }
                contentItem: QQC2.Label {
                    id: label
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: tab.text
                    color: Tokens.textPrimary
                    font: tab.on ? Tokens.bodyStrong : Tokens.body
                }
            }
        }
    }
}

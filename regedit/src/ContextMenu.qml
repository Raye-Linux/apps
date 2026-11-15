// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// A menu of the menu bar or of a right click. items is a list of
// { text, shortcut, enabled, separator, action }; action is called on a press.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

QQC2.Popup {
    id: menu

    property var items: []

    padding: Tokens.sizeMenuPadding
    modal: false
    focus: true
    closePolicy: QQC2.Popup.CloseOnEscape | QQC2.Popup.CloseOnPressOutside

    function openAt(item, x, y) {
        parent = item
        menu.x = x
        menu.y = y
        open()
    }

    background: Rectangle {
        radius: Tokens.radiusOverlay
        color: Tokens.bgTertiary
        border.color: Tokens.strokeSurface
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 0
        Repeater {
            model: menu.items
            delegate: Loader {
                id: entry
                required property var modelData
                Layout.fillWidth: true
                Layout.minimumWidth: 200
                sourceComponent: modelData.separator ? separator : choice

                Component {
                    id: separator
                    Item {
                        implicitHeight: Tokens.s3
                        Rectangle {
                            anchors.centerIn: parent
                            width: parent.width - 2 * Tokens.s2
                            height: 1
                            color: Tokens.strokeDivider
                        }
                    }
                }
                Component {
                    id: choice
                    QQC2.AbstractButton {
                        implicitHeight: Tokens.sizeMenuItem
                        enabled: entry.modelData.enabled !== false
                        hoverEnabled: true
                        text: entry.modelData.text
                        onClicked: {
                            menu.close()
                            entry.modelData.action()
                        }
                        background: Rectangle {
                            radius: Tokens.radiusControl
                            color: parent.pressed ? Tokens.fillSubtlePressed : parent.hovered ? Tokens.fillSubtleHover : "transparent"
                        }
                        contentItem: RowLayout {
                            spacing: Tokens.s7
                            QQC2.Label {
                                Layout.fillWidth: true
                                leftPadding: Tokens.s4
                                verticalAlignment: Text.AlignVCenter
                                text: entry.modelData.text
                                color: entry.modelData.enabled !== false ? Tokens.textPrimary : Tokens.textDisabled
                                font: Tokens.body
                            }
                            QQC2.Label {
                                rightPadding: Tokens.s4
                                visible: !!entry.modelData.shortcut
                                text: entry.modelData.shortcut || ""
                                color: Tokens.textSecondary
                                font: Tokens.caption
                            }
                        }
                    }
                }
            }
        }
    }
}

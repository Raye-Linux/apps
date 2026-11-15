// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// A small menu. items is a list of { text, checked, separator, action }.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

QQC2.Popup {
    id: menu

    property var items: []
    property int minimumWidth: 200

    padding: Tokens.sizeMenuPadding
    focus: true
    closePolicy: QQC2.Popup.CloseOnEscape | QQC2.Popup.CloseOnPressOutside

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Tokens.durationFast }
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
                Layout.minimumWidth: menu.minimumWidth
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
                        id: item
                        implicitHeight: Tokens.sizeMenuItem
                        hoverEnabled: true
                        text: entry.modelData.text
                        onClicked: {
                            menu.close()
                            entry.modelData.action()
                        }
                        background: Rectangle {
                            radius: Tokens.radiusControl
                            color: item.pressed ? Tokens.fillSubtlePressed : item.hovered ? Tokens.fillSubtleHover : "transparent"
                        }
                        contentItem: RowLayout {
                            spacing: Tokens.s3
                            Item {
                                Layout.leftMargin: Tokens.s3
                                implicitWidth: 16
                                implicitHeight: 16
                                Glyph {
                                    anchors.fill: parent
                                    visible: entry.modelData.checked === true
                                    kind: "check"
                                    color: Tokens.accentText
                                }
                            }
                            QQC2.Label {
                                Layout.fillWidth: true
                                Layout.rightMargin: Tokens.s4
                                text: item.text
                                color: Tokens.textPrimary
                                font: Tokens.body
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }
        }
    }
}

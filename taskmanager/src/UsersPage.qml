// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Users: what each person with programs running is using. The system's
// own accounts are left out, as Windows leaves out its services.

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui
import org.raye.taskmanager

Item {
    id: page

    required property ProcessModel model
    readonly property int figureWidth: 112

    ColumnLayout {
        anchors.fill: parent
        spacing: Tokens.s3

        QQC2.Label {
            Layout.preferredHeight: Tokens.sizeControlHeight
            verticalAlignment: Text.AlignVCenter
            text: i18nc("@title", "Users")
            color: Tokens.textPrimary
            font: Tokens.subtitle
        }

        Card {
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: 0

            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    HeaderCell { Layout.fillWidth: true; alignRight: false; enabled: false; text: i18nc("@title:column", "User") }
                    HeaderCell { Layout.preferredWidth: page.figureWidth; enabled: false; text: i18nc("@title:column", "Processes") }
                    HeaderCell { Layout.preferredWidth: page.figureWidth; enabled: false; text: i18nc("@title:column share of the processor", "CPU") }
                    HeaderCell { Layout.preferredWidth: page.figureWidth; enabled: false; text: i18nc("@title:column memory in use", "Memory") }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Tokens.strokeDivider }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: page.visible ? page.model.users : []
                    delegate: Item {
                        id: row
                        required property var modelData
                        width: ListView.view.width
                        height: 56
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Tokens.s4
                            spacing: 0
                            // the person's initial, in the accent, like the account pictures elsewhere
                            Rectangle {
                                Layout.preferredWidth: 32
                                Layout.preferredHeight: 32
                                radius: 16
                                color: Qt.alpha(Tokens.accent, 0.25)
                                QQC2.Label {
                                    anchors.centerIn: parent
                                    text: row.modelData.fullName.charAt(0).toUpperCase()
                                    color: Tokens.textPrimary
                                    font: Tokens.bodyStrong
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: Tokens.s4
                                spacing: 0
                                QQC2.Label {
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                    text: row.modelData.fullName
                                    color: Tokens.textPrimary
                                    font: Tokens.body
                                }
                                QQC2.Label {
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                    text: row.modelData.self ? i18nc("@info the account in use, and its login", "%1 (you)", row.modelData.name) : row.modelData.name
                                    color: Tokens.textSecondary
                                    font: Tokens.caption
                                }
                            }
                            Repeater {
                                model: [String(row.modelData.count), Fmt.percent(row.modelData.cpu), Fmt.bytes(row.modelData.memory)]
                                delegate: QQC2.Label {
                                    required property var modelData
                                    Layout.preferredWidth: page.figureWidth
                                    rightPadding: Tokens.s3
                                    horizontalAlignment: Text.AlignRight
                                    text: modelData
                                    color: Tokens.textPrimary
                                    font: Tokens.body
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

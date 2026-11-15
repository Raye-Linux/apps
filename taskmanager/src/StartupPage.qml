// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Startup apps: what starts when the person signs in, and the switch to
// stop it. Turning one off writes the person's own copy of the entry;
// the system's file is never touched (see autostart.h).
//
// "Startup impact" is the processor time the entry has used since sign-in
// -- the nearest figure Linux keeps. An entry that is not running has none.

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.raye.ui
import org.raye.taskmanager

Item {
    id: page

    property bool active: false
    property string search: ""
    property int currentRow: -1
    property bool currentEnabled: false
    property string currentName: ""

    readonly property int statusWidth: 120
    readonly property int impactWidth: 140

    onActiveChanged: if (active) startup.refresh()

    StartupModel {
        id: startup
    }

    function impactWord(level) {
        switch (level) {
        case StartupModel.None: return i18nc("@info startup impact", "None")
        case StartupModel.Low: return i18nc("@info startup impact", "Low")
        case StartupModel.Medium: return i18nc("@info startup impact", "Medium")
        case StartupModel.High: return i18nc("@info startup impact", "High")
        }
        return i18nc("@info startup impact of an entry that is not running", "Not measured")
    }

    function toggle() {
        if (!startup.setEnabled(currentRow, !currentEnabled))
            note.message = i18nc("@info", "The change could not be saved.")
        else
            currentEnabled = !currentEnabled
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Tokens.s3

        RowLayout {
            Layout.fillWidth: true
            QQC2.Label {
                Layout.fillWidth: true
                text: i18nc("@title", "Startup apps")
                color: Tokens.textPrimary
                font: Tokens.subtitle
            }
            CommandButton {
                enabled: page.currentRow >= 0
                text: page.currentEnabled ? i18nc("@action:button stop an app starting at sign-in", "Disable")
                                          : i18nc("@action:button let an app start at sign-in", "Enable")
                onClicked: page.toggle()
            }
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
                    HeaderCell { Layout.fillWidth: true; alignRight: false; enabled: false; text: i18nc("@title:column", "Name") }
                    HeaderCell { Layout.preferredWidth: page.statusWidth; alignRight: false; enabled: false; text: i18nc("@title:column", "Status") }
                    HeaderCell { Layout.preferredWidth: page.impactWidth; alignRight: false; enabled: false; text: i18nc("@title:column", "Startup impact") }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Tokens.strokeDivider }

                ListView {
                    id: list
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: startup
                    boundsBehavior: Flickable.StopAtBounds
                    QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
                    delegate: Item {
                        id: row
                        required property int index
                        required property string name
                        required property string icon
                        required property string comment
                        required property bool startsAtSignIn
                        required property int impact
                        readonly property bool shown: page.search === "" || name.toLowerCase().includes(page.search.toLowerCase())
                        width: ListView.view.width
                        height: shown ? Tokens.sizeListItem : 0
                        visible: shown

                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 1
                            radius: Tokens.radiusControl
                            color: page.currentRow === row.index ? Tokens.fillSubtleSelected : hover.hovered ? Tokens.fillSubtleHover : "transparent"
                        }
                        HoverHandler { id: hover }
                        TapHandler {
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            onTapped: (point, button) => {
                                page.currentRow = row.index
                                page.currentEnabled = row.startsAtSignIn
                                page.currentName = row.name
                                if (button === Qt.RightButton)
                                    menu.openAt(row, point.position.x, point.position.y)
                            }
                        }
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Tokens.s4
                            spacing: 0
                            Kirigami.Icon {
                                Layout.preferredWidth: 20
                                Layout.preferredHeight: 20
                                source: row.icon !== "" ? row.icon : "application-x-executable"
                                fallback: "application-x-executable"
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: Tokens.s3
                                spacing: 0
                                QQC2.Label {
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                    text: row.name
                                    color: Tokens.textPrimary
                                    font: Tokens.body
                                }
                            }
                            QQC2.Label {
                                Layout.preferredWidth: page.statusWidth
                                leftPadding: Tokens.s3
                                text: row.startsAtSignIn ? i18nc("@info:status starts at sign-in", "Enabled") : i18nc("@info:status does not start at sign-in", "Disabled")
                                color: row.startsAtSignIn ? Tokens.textPrimary : Tokens.textSecondary
                                font: Tokens.body
                            }
                            QQC2.Label {
                                Layout.preferredWidth: page.impactWidth
                                leftPadding: Tokens.s3
                                text: page.impactWord(row.impact)
                                color: row.impact === StartupModel.High ? Tokens.statusCaution : Tokens.textPrimary
                                font: Tokens.body
                            }
                        }
                    }
                }
            }

            QQC2.Label {
                anchors.centerIn: parent
                visible: startup.count === 0
                text: i18nc("@info", "Nothing starts at sign-in.")
                color: Tokens.textSecondary
                font: Tokens.body
            }
        }
    }

    ContextMenu {
        id: menu
        items: [
            { text: page.currentEnabled ? i18nc("@action:inmenu", "Disable") : i18nc("@action:inmenu", "Enable"),
              action: () => page.toggle() },
        ]
    }

    InfoBar {
        id: note
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Tokens.s4
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width - 2 * Tokens.s4, Tokens.sizeTooltipMaxWidth * 2)
        visible: message !== ""
        severity: InfoBar.Warning
        onCloseClicked: message = ""
    }
}

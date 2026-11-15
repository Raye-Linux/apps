// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Services: the system's services, and starting, stopping or restarting
// one. Each of the three asks for an administrator's password through
// polkit; a refusal is said on the page, never swallowed.

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui
import org.raye.taskmanager

Item {
    id: page

    property bool active: false
    property string search: ""
    property string currentUnit: ""
    property string currentName: ""
    property string currentState: ""

    readonly property int pidWidth: 80
    readonly property int stateWidth: 112
    readonly property int startupWidth: 160

    ServicesModel {
        id: services
        active: page.active
        filter: page.search
        onControlFinished: (unit, verb, ok, message) => {
            if (ok)
                return
            // polkit's refusal and a cancelled password prompt both come
            // back as an authorisation error
            note.message = message.indexOf("AccessDenied") >= 0 || message.indexOf("Authorization") >= 0 || message.indexOf("InteractiveAuthorizationRequired") >= 0
                ? i18nc("@info", "You need root privileges to change “%1”.", unit.replace(/\.service$/, ""))
                : i18nc("@info", "“%1” could not be changed.", unit.replace(/\.service$/, ""))
        }
    }

    function stateWord(s) {
        switch (s) {
        case "running": return i18nc("@info:status a service", "Running")
        case "finished": return i18nc("@info:status a service that ran once and is done", "Done")
        case "failed": return i18nc("@info:status a service", "Failed")
        case "starting": return i18nc("@info:status a service", "Starting")
        case "stopping": return i18nc("@info:status a service", "Stopping")
        }
        return i18nc("@info:status a service", "Stopped")
    }

    function startupWord(s) {
        switch (s) {
        case "enabled": return i18nc("@info a service that starts with the computer", "Automatic")
        case "disabled": return i18nc("@info a service started only when asked", "Manual")
        case "masked": return i18nc("@info a service that cannot be started", "Disabled")
        case "static": return i18nc("@info a service started by another one", "By another service")
        }
        return s === "" ? "—" : s
    }

    readonly property bool currentRuns: currentState === "running" || currentState === "starting"

    ColumnLayout {
        anchors.fill: parent
        spacing: Tokens.s3

        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.s2
            QQC2.Label {
                Layout.fillWidth: true
                text: i18nc("@title", "Services")
                color: Tokens.textPrimary
                font: Tokens.subtitle
            }
            CommandButton {
                enabled: page.currentUnit !== "" && !page.currentRuns
                text: i18nc("@action:button a service", "Start")
                onClicked: services.control(page.currentUnit, "start")
            }
            CommandButton {
                enabled: page.currentUnit !== "" && page.currentRuns
                text: i18nc("@action:button a service", "Stop")
                onClicked: services.control(page.currentUnit, "stop")
            }
            CommandButton {
                enabled: page.currentUnit !== "" && page.currentRuns
                text: i18nc("@action:button a service", "Restart")
                onClicked: services.control(page.currentUnit, "restart")
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
                    Layout.rightMargin: Tokens.s3
                    spacing: 0
                    HeaderCell { Layout.preferredWidth: 220; alignRight: false; enabled: false; text: i18nc("@title:column", "Name") }
                    HeaderCell { Layout.preferredWidth: page.pidWidth; enabled: false; text: i18nc("@title:column process number", "PID") }
                    HeaderCell { Layout.fillWidth: true; alignRight: false; enabled: false; text: i18nc("@title:column", "Description") }
                    HeaderCell { Layout.preferredWidth: page.stateWidth; alignRight: false; enabled: false; text: i18nc("@title:column", "Status") }
                    HeaderCell { Layout.preferredWidth: page.startupWidth; alignRight: false; enabled: false; text: i18nc("@title:column how a service starts", "Startup") }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Tokens.strokeDivider }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: services
                    reuseItems: true
                    boundsBehavior: Flickable.StopAtBounds
                    QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
                    rightMargin: Tokens.s3
                    delegate: Item {
                        id: row
                        required property string name
                        required property string unit
                        required property string description
                        required property string state
                        required property int pid
                        required property string startup
                        required property bool busy
                        width: ListView.view.width - Tokens.s3
                        height: Tokens.sizeTreeItem
                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 1
                            radius: Tokens.radiusControl
                            color: page.currentUnit === row.unit ? Tokens.fillSubtleSelected : hover.hovered ? Tokens.fillSubtleHover : "transparent"
                        }
                        HoverHandler { id: hover }
                        TapHandler {
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            onTapped: (point, button) => {
                                page.currentUnit = row.unit
                                page.currentName = row.name
                                page.currentState = row.state
                                if (button === Qt.RightButton)
                                    menu.openAt(row, point.position.x, point.position.y)
                            }
                        }
                        // the selected service's state follows it as it starts or stops
                        Binding {
                            target: page
                            property: "currentState"
                            value: row.state
                            when: page.currentUnit === row.unit
                        }
                        RowLayout {
                            anchors.fill: parent
                            spacing: 0
                            QQC2.Label {
                                Layout.preferredWidth: 220
                                leftPadding: Tokens.s3
                                elide: Text.ElideRight
                                text: row.name
                                color: Tokens.textPrimary
                                font: Tokens.body
                            }
                            QQC2.Label {
                                Layout.preferredWidth: page.pidWidth
                                rightPadding: Tokens.s3
                                horizontalAlignment: Text.AlignRight
                                text: row.pid > 0 ? String(row.pid) : ""
                                color: Tokens.textPrimary
                                font: Tokens.body
                            }
                            QQC2.Label {
                                Layout.fillWidth: true
                                leftPadding: Tokens.s3
                                elide: Text.ElideRight
                                text: row.description
                                color: Tokens.textSecondary
                                font: Tokens.body
                            }
                            QQC2.Label {
                                Layout.preferredWidth: page.stateWidth
                                leftPadding: Tokens.s3
                                text: row.busy ? i18nc("@info:status waiting for an answer", "Working…") : page.stateWord(row.state)
                                color: row.state === "failed" ? Tokens.statusCritical : row.state === "running" ? Tokens.textPrimary : Tokens.textSecondary
                                font: Tokens.body
                            }
                            QQC2.Label {
                                Layout.preferredWidth: page.startupWidth
                                leftPadding: Tokens.s3
                                elide: Text.ElideRight
                                text: page.startupWord(row.startup)
                                color: Tokens.textSecondary
                                font: Tokens.body
                            }
                        }
                    }
                }
            }

            QQC2.Label {
                anchors.centerIn: parent
                visible: services.loaded && services.count === 0
                text: services.unavailable ? i18nc("@info", "The list of services could not be read.")
                                           : i18nc("@info a search found nothing", "No results")
                color: Tokens.textSecondary
                font: Tokens.body
            }
        }
    }

    ContextMenu {
        id: menu
        items: [
            { text: i18nc("@action:inmenu a service", "Start"), enabled: !page.currentRuns, action: () => services.control(page.currentUnit, "start") },
            { text: i18nc("@action:inmenu a service", "Stop"), enabled: page.currentRuns, action: () => services.control(page.currentUnit, "stop") },
            { text: i18nc("@action:inmenu a service", "Restart"), enabled: page.currentRuns, action: () => services.control(page.currentUnit, "restart") },
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

// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// "Run new task": a command, and whether to run it as root.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui
import org.raye.taskmanager

QQC2.Dialog {
    id: sheet

    required property ProcessModel model

    modal: true
    anchors.centerIn: parent
    padding: Tokens.s6
    closePolicy: QQC2.Popup.CloseOnEscape
    // "", "waiting", or what to tell after a run as root
    property string phase: ""
    property string problem: ""
    property string output: ""

    onOpened: {
        problem = ""
        output = ""
        phase = ""
        command.forceActiveFocus()
    }

    QQC2.Overlay.modal: Rectangle { color: Tokens.bgSmoke }

    // a solid fill: the card colour is see-through, made for a card on the page
    background: Rectangle {
        radius: Tokens.radiusOverlay
        color: Tokens.bgTertiary
        border.width: 1
        border.color: Tokens.strokeSurface
    }

    function run() {
        problem = ""
        output = ""
        const result = sheet.model.runTask(command.text, admin.checked)
        if (result === "started") {
            command.clear()
            admin.checked = false
            sheet.close()
        } else if (result === "waiting") {
            phase = "waiting"
        } else if (result === "window") {
            problem = i18nc("@info", "Programs with a window can't run as root. Open it the usual way: it asks for the password when it needs it.")
        } else {
            problem = i18nc("@info", "That program could not be found.")
        }
    }

    Connections {
        target: sheet.model
        function onRootTaskFinished(code, text) {
            if (sheet.phase !== "waiting")
                return
            sheet.phase = ""
            sheet.output = text
            if (code === 0 && text === "") {
                command.clear()
                admin.checked = false
                sheet.close()
            } else if (code === 0) {
                sheet.phase = "done"
            } else if (code === 126) {
                sheet.problem = i18nc("@info", "The password wasn't given, so nothing ran.")
            } else if (code === 127) {
                sheet.problem = i18nc("@info", "Running it as root wasn't allowed.")
            } else {
                sheet.problem = i18nc("@info", "It stopped with error code %1.", code)
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: Tokens.s4
        width: 440

        QQC2.Label {
            text: i18nc("@title", "Run new task")
            color: Tokens.textPrimary
            font: Tokens.subtitle
        }
        QQC2.Label {
            Layout.fillWidth: true
            Layout.preferredWidth: 440
            wrapMode: Text.Wrap
            text: i18nc("@info", "Type the name of a program and Raye will open it for you.")
            color: Tokens.textSecondary
            font: Tokens.body
        }
        SearchField {
            id: command
            Layout.fillWidth: true
            showGlyph: false
            placeholderText: i18nc("@info:placeholder", "For example: notepad")
            enabled: sheet.phase !== "waiting"
            onAccepted: if (enabled) sheet.run()
        }
        QQC2.Label {
            Layout.fillWidth: true
            Layout.preferredWidth: 440
            visible: text !== ""
            wrapMode: Text.Wrap
            text: sheet.phase === "waiting" ? i18nc("@info", "Waiting for the root password…")
                : sheet.phase === "done" ? i18nc("@info", "Done. It wrote:")
                : sheet.problem
            color: sheet.problem !== "" ? Tokens.statusCritical : Tokens.textSecondary
            font: Tokens.caption
        }
        QQC2.Label {
            Layout.fillWidth: true
            Layout.preferredWidth: 440
            Layout.maximumHeight: 160
            visible: sheet.output !== ""
            clip: true
            wrapMode: Text.WrapAnywhere
            elide: Text.ElideLeft
            text: sheet.output
            color: Tokens.textPrimary
            font.family: "monospace"
            font.pixelSize: 12
        }
        CheckRow {
            id: admin
            enabled: sheet.phase !== "waiting"
            text: i18nc("@option:check", "Run this task as root")
        }
        QQC2.Label {
            Layout.fillWidth: true
            Layout.preferredWidth: 440
            visible: admin.checked
            wrapMode: Text.Wrap
            text: i18nc("@info", "For commands without a window. What they write shows up here when they finish.")
            color: Tokens.textSecondary
            font: Tokens.caption
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Tokens.s2
            spacing: Tokens.s2
            Item { Layout.fillWidth: true }
            AccentButton {
                accent: false
                text: sheet.phase === "done" ? i18nc("@action:button", "Close") : i18nc("@action:button", "Cancel")
                onClicked: sheet.close()
            }
            AccentButton {
                enabled: command.text.trim() !== "" && sheet.phase !== "waiting"
                text: i18nc("@action:button start the program", "OK")
                onClicked: sheet.run()
            }
        }
    }
}

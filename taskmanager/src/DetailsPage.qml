// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Details: every process on its own row, with its number, owner, threads
// and priority, and the actions Windows puts in its right-click menu --
// end task, end process tree, set priority, efficiency mode.
//
// The command line is read for the selected row only.

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui
import org.raye.taskmanager

Item {
    id: page

    required property ProcessModel model
    property string search: ""
    property int currentPid: 0
    property string currentName: ""
    property int currentPriority: 2
    property bool currentIsEfficient: false

    signal runTaskRequested()

    onSearchChanged: model.filter = search

    readonly property var columns: [
        { column: "name", text: i18nc("@title:column", "Name"), width: -1 },
        { column: "pid", text: i18nc("@title:column process number", "PID"), width: 72 },
        { column: "status", text: i18nc("@title:column", "Status"), width: 104 },
        { column: "user", text: i18nc("@title:column", "User name"), width: 112 },
        { column: "cpu", text: i18nc("@title:column share of the processor", "CPU"), width: 64 },
        { column: "memory", text: i18nc("@title:column memory in use", "Memory"), width: 96 },
        { column: "threads", text: i18nc("@title:column", "Threads"), width: 72 },
        { column: "priority", text: i18nc("@title:column", "Priority"), width: 112 },
    ]

    function priorityWord(p) {
        switch (p) {
        case 0: return i18nc("@item process priority", "Low")
        case 1: return i18nc("@item process priority", "Below normal")
        case 3: return i18nc("@item process priority", "Above normal")
        case 4: return i18nc("@item process priority", "High")
        }
        return i18nc("@item process priority", "Normal")
    }

    function stateWord(s) {
        switch (s) {
        case "R": return i18nc("@info:status a process using the processor", "Running")
        case "S": return i18nc("@info:status a process waiting for something", "Waiting")
        case "D": return i18nc("@info:status a process waiting on the disk", "On the disk")
        case "T": return i18nc("@info:status a process stopped by a signal", "Suspended")
        case "t": return i18nc("@info:status a process held by a debugger", "Held")
        case "Z": return i18nc("@info:status a process that has ended and not been collected", "Ended")
        case "I": return i18nc("@info:status an idle kernel thread", "Idle")
        }
        return s
    }

    // the Processes page sends a row here
    function reveal(pid) {
        model.filter = ""
        currentPid = pid
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Tokens.s3

        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.s2
            QQC2.Label {
                Layout.fillWidth: true
                text: i18nc("@title", "Details")
                color: Tokens.textPrimary
                font: Tokens.subtitle
            }
            CommandButton {
                glyph: "run"
                text: i18nc("@action:button start a program by its command", "Run new task")
                onClicked: page.runTaskRequested()
            }
            CommandButton {
                glyph: "end"
                enabled: page.currentPid > 0
                text: i18nc("@action:button ask a process to close", "End task")
                onClicked: page.endTask(false)
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
                    Repeater {
                        model: page.columns
                        delegate: HeaderCell {
                            required property var modelData
                            Layout.fillWidth: modelData.width < 0
                            Layout.preferredWidth: modelData.width < 0 ? 160 : modelData.width
                            column: modelData.column
                            alignRight: ["pid", "cpu", "memory", "threads"].includes(modelData.column)
                            text: modelData.text
                            sortColumn: page.model.sortColumn
                            sortDescending: page.model.sortDescending
                            onSortRequested: (c, d) => {
                                page.model.sortColumn = c
                                page.model.sortDescending = d
                            }
                        }
                    }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Tokens.strokeDivider }

                ListView {
                    id: list
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    // nothing to lay out while another page is shown
                    model: page.visible ? page.model : null
                    reuseItems: true
                    boundsBehavior: Flickable.StopAtBounds
                    QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
                    rightMargin: Tokens.s3
                    delegate: Item {
                        id: row
                        required property int pid
                        required property string name
                        required property string state
                        required property string user
                        required property real cpuPercent
                        required property double memoryBytes
                        required property int threads
                        required property int priority
                        required property bool efficiency
                        width: ListView.view.width - Tokens.s3
                        height: Tokens.sizeTreeItem

                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 1
                            radius: Tokens.radiusControl
                            color: page.currentPid === row.pid ? Tokens.fillSubtleSelected : hover.hovered ? Tokens.fillSubtleHover : "transparent"
                        }
                        HoverHandler { id: hover }
                        TapHandler {
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            onTapped: (point, button) => {
                                page.currentPid = row.pid
                                page.currentName = row.name
                                page.currentPriority = row.priority
                                page.currentIsEfficient = row.efficiency
                                if (button === Qt.RightButton)
                                    menu.openAt(row, point.position.x, point.position.y)
                            }
                        }
                        RowLayout {
                            anchors.fill: parent
                            spacing: 0
                            // over the fixed list of columns, so a new figure changes
                            // a text and never rebuilds the cells
                            Repeater {
                                model: page.columns
                                delegate: QQC2.Label {
                                    required property var modelData
                                    required property int index
                                    readonly property var spec: modelData
                                    Layout.fillWidth: spec.width < 0
                                    Layout.preferredWidth: spec.width < 0 ? 160 : spec.width
                                    leftPadding: Tokens.s3
                                    rightPadding: Tokens.s3
                                    elide: Text.ElideRight
                                    horizontalAlignment: ["pid", "cpu", "memory", "threads"].includes(spec.column) ? Text.AlignRight : Text.AlignLeft
                                    text: {
                                        switch (spec.column) {
                                        case "name": return row.name
                                        case "pid": return String(row.pid)
                                        case "status": return page.stateWord(row.state)
                                        case "user": return row.user
                                        case "cpu": return Fmt.number(row.cpuPercent, 0)
                                        case "memory": return Fmt.bytes(row.memoryBytes)
                                        case "threads": return String(row.threads)
                                        }
                                        return page.priorityWord(row.priority)
                                    }
                                    color: Tokens.textPrimary
                                    font: Tokens.body
                                }
                            }
                        }
                    }
                }
            }
        }

        // the selected process's command line, read for it alone
        QQC2.Label {
            Layout.fillWidth: true
            visible: page.currentPid > 0
            elide: Text.ElideMiddle
            text: page.currentPid > 0 ? i18nc("@info the command line of the selected process", "Command line: %1", page.model.commandLine(page.currentPid) || "—") : ""
            color: Tokens.textSecondary
            font: Tokens.caption
        }
    }

    ContextMenu {
        id: menu
        items: [
            { text: i18nc("@action:inmenu", "End task"), action: () => page.endTask(false) },
            { text: i18nc("@action:inmenu end a process and every process it started", "End process tree"), action: () => treeSheet.open() },
            { separator: true },
            { text: i18nc("@action:inmenu", "Efficiency mode"), enabled: !page.currentIsEfficient, action: () => efficiencySheet.open() },
            { separator: true },
            { text: i18nc("@action:inmenu set the priority to", "Priority: High"), enabled: page.currentPriority !== 4, action: () => page.setPriority(4) },
            { text: i18nc("@action:inmenu set the priority to", "Priority: Above normal"), enabled: page.currentPriority !== 3, action: () => page.setPriority(3) },
            { text: i18nc("@action:inmenu set the priority to", "Priority: Normal"), enabled: page.currentPriority !== 2, action: () => page.setPriority(2) },
            { text: i18nc("@action:inmenu set the priority to", "Priority: Below normal"), enabled: page.currentPriority !== 1, action: () => page.setPriority(1) },
            { text: i18nc("@action:inmenu set the priority to", "Priority: Low"), enabled: page.currentPriority !== 0, action: () => page.setPriority(0) },
        ]
    }

    function setPriority(p) {
        const result = model.setPriority(currentPid, p)
        if (result === ProcessModel.PriorityRefused)
            note.message = i18nc("@info", "That process is no longer there.")
        else if (result === ProcessModel.PriorityAsked)
            note.message = i18nc("@info", "Raising a priority needs root privileges.")
        else
            currentPriority = p
    }

    ConfirmSheet {
        id: treeSheet
        parent: page
        heading: i18nc("@title", "End the process tree of “%1”?", page.currentName)
        body: i18nc("@info", "Every program it started is asked to close too. Anything they have not saved may be lost.")
        confirmText: i18nc("@action:button", "End process tree")
        destructive: true
        onConfirmed: {
            const asked = page.model.endTree(page.currentPid)
            note.message = asked > 0 ? i18ncp("@info", "%1 process was asked to close.", "%1 processes were asked to close.", asked)
                                     : i18nc("@info", "That process has already ended.")
        }
    }

    ConfirmSheet {
        id: efficiencySheet
        parent: page
        heading: i18nc("@title", "Turn on efficiency mode for “%1”?", page.currentName)
        body: page.model.efficiencyIsReversible
              ? i18nc("@info", "It will only run when nothing else needs the processor. You can turn it off later.")
              : i18nc("@info", "It will only run when nothing else needs the processor. You can't turn this off until it restarts.")
        confirmText: i18nc("@action:button", "Turn on")
        onConfirmed: page.model.setEfficiency(page.currentPid, true)
    }

    ConfirmSheet {
        id: forceSheet
        parent: page
        heading: i18nc("@title", "Force “%1” to close?", page.currentName)
        body: i18nc("@info", "It was asked to close and has not. Forcing it loses anything it has not saved.")
        confirmText: i18nc("@action:button", "Force to close")
        destructive: true
        onConfirmed: page.endTask(true)
    }

    function endTask(insisted) {
        const step = model.endTask(currentPid, insisted)
        if (step === ProcessModel.Terminate)
            note.message = i18nc("@info", "“%1” was asked to close.", currentName)
        else if (step === ProcessModel.OfferForce)
            forceSheet.open()
        else if (step === ProcessModel.Wait)
            note.message = i18nc("@info", "“%1” was already asked to close; giving it a moment.", currentName)
        else if (step === ProcessModel.Gone)
            note.message = i18nc("@info", "That process has already ended.")
    }

    InfoBar {
        id: note
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Tokens.s6
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width - 2 * Tokens.s4, Tokens.sizeTooltipMaxWidth * 2)
        visible: message !== ""
        severity: InfoBar.Information
        onCloseClicked: message = ""
    }
}

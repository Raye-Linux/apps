// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The Processes page: apps with their processes under them, then the
// background processes, and the two actions on the selected row.
//
// No Network or GPU column: the kernel keeps no per-process byte count
// for the network, and no per-process figure for the graphics card that
// every driver gives. The machine's totals are on the Performance page.
// Disk is read from /proc/<pid>/io, which only the owner may read: a row
// of another user shows a dash, not a 0 that was never measured.

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui
import org.raye.taskmanager

Item {
    id: page

    required property ProcessModel model
    required property PerformanceModel performance
    property alias search: groups.filter

    // Kept here, and NOT read back out of the delegate: the list
    // reuses its delegates, so the item under currentIndex may already
    // have been handed to another row by the time it is read.
    property string currentKey: ""
    property string currentKind: ""
    property int currentPid: 0
    property var currentPids: []
    property string currentName: ""
    property bool currentIsEfficient: false

    // one width per column, for the headings and the rows alike
    readonly property int statusWidth: 136
    readonly property int figureWidth: 96

    signal runTaskRequested()

    ProcessGroupsModel {
        id: groups
        // rebuilt after every reading only while the page is shown
        source: page.visible ? page.model : null
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Tokens.s3

        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.s2

            QQC2.Label {
                Layout.fillWidth: true
                text: i18nc("@title the page of running programs", "Processes")
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
            CommandButton {
                glyph: "leaf"
                enabled: page.currentPid > 0 && !page.currentIsEfficient
                text: i18nc("@action:button put a process in the efficiency mode", "Efficiency mode")
                onClicked: efficiencySheet.open()
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
                    id: headings
                    Layout.fillWidth: true
                    Layout.rightMargin: Tokens.s3
                    spacing: 0

                    HeaderCell {
                        Layout.fillWidth: true
                        column: "name"
                        alignRight: false
                        text: i18nc("@title:column the process's name", "Name")
                        sortColumn: groups.sortColumn
                        sortDescending: groups.sortDescending
                        total: " "
                        onSortRequested: (c, d) => page.sortBy(c, d)
                    }
                    HeaderCell {
                        Layout.preferredWidth: page.statusWidth
                        column: "status"
                        enabled: false
                        text: i18nc("@title:column what the process is doing", "Status")
                        total: " "
                    }
                    HeaderCell {
                        Layout.preferredWidth: page.figureWidth
                        column: "cpu"
                        text: i18nc("@title:column share of the processor", "CPU")
                        total: Fmt.percent(page.performance.cpuPercent)
                        busy: page.performance.cpuPercent >= 80
                        sortColumn: groups.sortColumn
                        sortDescending: groups.sortDescending
                        onSortRequested: (c, d) => page.sortBy(c, d)
                    }
                    HeaderCell {
                        Layout.preferredWidth: page.figureWidth
                        column: "memory"
                        text: i18nc("@title:column memory in use", "Memory")
                        readonly property var mem: page.performance.memory
                        total: Fmt.percent(mem.total > 0 ? mem.inUse * 100 / mem.total : 0)
                        busy: mem.total > 0 && mem.inUse / mem.total >= 0.8
                        sortColumn: groups.sortColumn
                        sortDescending: groups.sortDescending
                        onSortRequested: (c, d) => page.sortBy(c, d)
                    }
                    HeaderCell {
                        Layout.preferredWidth: page.figureWidth
                        column: "disk"
                        text: i18nc("@title:column disk reads and writes", "Disk")
                        total: Fmt.percent(page.performance.diskPercent)
                        busy: page.performance.diskPercent >= 80
                        sortColumn: groups.sortColumn
                        sortDescending: groups.sortDescending
                        onSortRequested: (c, d) => page.sortBy(c, d)
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 1
                    color: Tokens.strokeDivider
                }

                ListView {
                    id: list
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: page.visible ? groups : null
                    reuseItems: true
                    boundsBehavior: Flickable.StopAtBounds
                    QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
                    rightMargin: Tokens.s3
                    delegate: GroupRow {
                        id: groupRow
                        width: ListView.view.width - Tokens.s3
                        statusWidth: page.statusWidth
                        figureWidth: page.figureWidth
                        selected: page.currentKey === key
                        required property var pids
                        onClicked: page.select(key, kind, pid, pids, name, efficiency)
                        onToggled: groups.toggle(key)
                        onMenuRequested: (x, y) => menu.openAt(groupRow, x, y)
                    }
                }
            }

            QQC2.Label {
                anchors.centerIn: parent
                visible: list.count === 0 && page.model.firstSampleTaken
                text: i18nc("@info a search found nothing", "No results")
                color: Tokens.textSecondary
                font: Tokens.body
            }
        }
    }

    ContextMenu {
        id: menu
        items: [
            { text: i18nc("@action:inmenu ask a process to close", "End task"), action: () => page.endTask(false) },
            { text: i18nc("@action:inmenu", "Efficiency mode"), enabled: !page.currentIsEfficient,
              action: () => efficiencySheet.open() },
            { separator: true },
            { text: i18nc("@action:inmenu open the Details page on this process", "Go to details"),
              action: () => page.detailsRequested(page.currentPid) },
        ]
    }

    signal detailsRequested(int pid)

    function sortBy(column, descending) {
        groups.sortColumn = column
        groups.sortDescending = descending
    }

    function select(key, kind, pid, pids, name, efficient) {
        currentKey = key
        currentKind = kind
        currentPid = pid
        currentPids = pids
        currentName = name
        currentIsEfficient = efficient
    }

    // ---- the two sheets -------------------------------------------

    ConfirmSheet {
        id: efficiencySheet
        parent: page
        heading: i18nc("@title", "Turn on efficiency mode for “%1”?", page.currentName)
        // Measured, not described: in the idle class a process gets
        // only the processor nobody else wants.
        body: page.model.efficiencyIsReversible
              ? i18nc("@info", "It will only run when nothing else needs the processor. You can turn it off later.")
              : i18nc("@info", "It will only run when nothing else needs the processor. You can't turn this off until it restarts.")
        confirmText: i18nc("@action:button", "Turn on")
        onConfirmed: {
            let refused = true, ioOnly = false
            for (const pid of page.currentPids) {
                const result = page.model.setEfficiency(pid, true)
                refused = refused && result === ProcessModel.Refused
                ioOnly = ioOnly || result === ProcessModel.IoOnly
            }
            if (refused)
                page.say(i18nc("@info", "That process is no longer there."))
            else if (ioOnly)
                page.say(i18nc("@info", "Only its disk priority could be lowered."))
        }
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

    InfoBar {
        id: note
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Tokens.s4
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width - 2 * Tokens.s4, Tokens.sizeTooltipMaxWidth * 2)
        visible: message !== ""
        severity: InfoBar.Information
        onCloseClicked: message = ""
    }

    function say(text) {
        note.message = text
    }

    // One press is a request to close; the sheet only appears when the
    // program has been asked and has not gone.
    function endTask(insisted) {
        const step = page.model.endTasks(page.currentPids, insisted)
        if (step === ProcessModel.Terminate)
            page.say(i18nc("@info", "“%1” was asked to close.", page.currentName))
        else if (step === ProcessModel.OfferForce)
            forceSheet.open()
        else if (step === ProcessModel.Wait)
            page.say(i18nc("@info", "“%1” was already asked to close; giving it a moment.", page.currentName))
        else if (step === ProcessModel.Gone)
            page.say(i18nc("@info", "That process has already ended."))
    }
}

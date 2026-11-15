// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The Performance page: a tile per device on the left, each with its last
// minute in small, and the chosen one in large on the right with its
// figures underneath, as in Windows 11.

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui
import org.raye.taskmanager

Item {
    id: page

    required property PerformanceModel performance
    property int processCount: 0
    property string current: "cpu"

    signal runTaskRequested()

    readonly property var mem: performance.memory
    readonly property var cpuInfo: performance.cpuInfo

    // one colour per kind of device, the same in the tile and the graph
    readonly property color cpuColor: Tokens.accent
    readonly property color memColor: Tokens.brandMagenta
    readonly property color diskColor: Tokens.statusSuccess
    readonly property color netColor: Tokens.statusCaution
    readonly property color gpuColor: Tokens.statusInfo

    function find(list, key) {
        for (const item of list)
            if (item.key === key)
                return item
        return null
    }

    readonly property var disk: find(performance.disks, current)
    readonly property var adapter: find(performance.adapters, current)
    readonly property var gpu: find(performance.gpus, current)

    function adapterTitle(a) {
        return a.wifi ? i18nc("@title a wireless network adapter", "Wi-Fi") : i18nc("@title a wired network adapter", "Ethernet")
    }
    function diskTitle(d) {
        return i18nc("@title a disk, its number and its name in the system", "Disk %1 (%2)", d.number, d.name)
    }
    function diskKind(d) {
        return d.removable ? i18nc("@info a removable disk", "Removable")
             : d.solidState ? i18nc("@info a solid-state disk", "SSD") : i18nc("@info a spinning disk", "HDD")
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Tokens.s3

        RowLayout {
            Layout.fillWidth: true
            QQC2.Label {
                Layout.fillWidth: true
                text: i18nc("@title the page of graphs", "Performance")
                color: Tokens.textPrimary
                font: Tokens.subtitle
            }
            CommandButton {
                glyph: "run"
                text: i18nc("@action:button start a program by its command", "Run new task")
                onClicked: page.runTaskRequested()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Tokens.s5

            // the tiles
            QQC2.ScrollView {
                Layout.preferredWidth: 232
                Layout.fillHeight: true
                contentWidth: availableWidth
                clip: true

                ColumnLayout {
                    width: parent.width
                    spacing: Tokens.s1

                    Tile {
                        key: "cpu"
                        title: i18nc("@title the processor", "CPU")
                        detail: Fmt.percent(page.performance.cpuPercent) + "  " + Fmt.ghz(page.performance.cpuMhz || page.cpuInfo.maxMhz)
                        values: page.performance.cpuHistory
                        tint: page.cpuColor
                    }
                    Tile {
                        key: "memory"
                        title: i18nc("@title", "Memory")
                        detail: i18nc("@info used of total, and the share", "%1/%2 (%3)", Fmt.bytes(page.mem.inUse), Fmt.bytes(page.mem.total),
                                      Fmt.percent(page.mem.total > 0 ? page.mem.inUse * 100 / page.mem.total : 0))
                        values: page.performance.memoryHistory
                        tint: page.memColor
                    }
                    Repeater {
                        model: page.performance.diskList
                        delegate: Tile {
                            required property var device
                            key: device.key
                            title: page.diskTitle(device)
                            detail: page.diskKind(device) + "\n" + Fmt.percent(device.active)
                            values: device.history
                            tint: page.diskColor
                        }
                    }
                    Repeater {
                        model: page.performance.adapterList
                        delegate: Tile {
                            required property var device
                            key: device.key
                            title: page.adapterTitle(device)
                            detail: device.name + "\n" + (device.connected
                                    ? i18nc("@info sent and received", "S: %1  R: %2", Fmt.bits(device.sent), Fmt.bits(device.received))
                                    : i18nc("@info:status a network adapter", "Not connected"))
                            values: device.history
                            maximum: Math.max(1e5, ...device.history) * 1.2
                            tint: page.netColor
                        }
                    }
                    Repeater {
                        model: page.performance.gpuList
                        delegate: Tile {
                            required property var device
                            required property int index
                            key: device.key
                            title: i18nc("@title a graphics card and its number", "GPU %1", index)
                            detail: device.name + "\n" + (device.busy >= 0 ? Fmt.percent(device.busy) : "")
                            values: device.history
                            tint: page.gpuColor
                        }
                    }
                }
            }

            // the chosen device
            QQC2.ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: availableWidth
                clip: true

                ColumnLayout {
                    width: parent.width
                    spacing: Tokens.s4

                    RowLayout {
                        Layout.fillWidth: true
                        QQC2.Label {
                            text: page.current === "cpu" ? i18nc("@title the processor", "CPU")
                                : page.current === "memory" ? i18nc("@title", "Memory")
                                : page.disk ? page.diskTitle(page.disk)
                                : page.adapter ? page.adapterTitle(page.adapter)
                                : page.gpu ? i18nc("@title a graphics card", "GPU") : ""
                            color: Tokens.textPrimary
                            font: Tokens.title
                        }
                        QQC2.Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            elide: Text.ElideRight
                            text: page.current === "cpu" ? page.cpuInfo.model
                                : page.current === "memory" ? Fmt.bytes(page.mem.total)
                                : page.disk ? page.disk.model
                                : page.adapter ? page.adapter.name
                                : page.gpu ? page.gpu.name : ""
                            color: Tokens.textPrimary
                            font: Tokens.bodyLarge
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        QQC2.Label {
                            Layout.fillWidth: true
                            text: page.current === "cpu" ? i18nc("@label above the processor graph", "% Utilization")
                                : page.current === "memory" ? i18nc("@label above the memory graph", "Memory usage")
                                : page.disk ? i18nc("@label above the disk graph", "Active time")
                                : page.adapter ? i18nc("@label above the network graph", "Throughput")
                                : i18nc("@label above the graphics card graph", "Utilization")
                            color: Tokens.textSecondary
                            font: Tokens.caption
                        }
                        QQC2.Label {
                            text: page.current === "memory" ? Fmt.bytes(page.mem.total)
                                : page.adapter ? Fmt.bits(bigGraph.maximum) : i18nc("@label the top of a graph", "100%")
                            color: Tokens.textSecondary
                            font: Tokens.caption
                        }
                    }

                    Graph {
                        id: bigGraph
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.max(200, page.height * 0.42)
                        visible: !page.gpu || page.gpu.busy >= 0
                        values: page.current === "cpu" ? page.performance.cpuHistory
                              : page.current === "memory" ? page.performance.memoryHistory
                              : page.disk ? page.disk.history
                              : page.adapter ? page.adapter.history
                              : page.gpu ? page.gpu.history : []
                        maximum: page.adapter ? Math.max(1e5, ...page.adapter.history) * 1.2 : 100
                        color: page.current === "cpu" ? page.cpuColor : page.current === "memory" ? page.memColor
                             : page.disk ? page.diskColor : page.adapter ? page.netColor : page.gpuColor
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        visible: !!page.gpu && page.gpu.busy < 0
                        wrapMode: Text.Wrap
                        text: i18nc("@info", "The driver of this graphics card does not say how busy it is.")
                        color: Tokens.textSecondary
                        font: Tokens.body
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        visible: bigGraph.visible
                        QQC2.Label {
                            Layout.fillWidth: true
                            text: i18nc("@label under a graph: how far back it goes", "60 seconds")
                            color: Tokens.textSecondary
                            font: Tokens.caption
                        }
                        QQC2.Label {
                            text: "0"
                            color: Tokens.textSecondary
                            font: Tokens.caption
                        }
                    }

                    // the figures: the big ones on the left, the facts on the right
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: Tokens.s3
                        spacing: Tokens.s8

                        GridLayout {
                            Layout.alignment: Qt.AlignTop
                            columns: 3
                            columnSpacing: Tokens.s7
                            rowSpacing: Tokens.s4
                            Repeater {
                                model: page.bigFigures.length
                                delegate: ColumnLayout {
                                    id: figure
                                    required property int index
                                    readonly property var pair: page.bigFigures[index] ?? ["", ""]
                                    // never narrower than it has been, so a figure that loses a
                                    // digit does not pull the facts on the right sideways
                                    property real widest: 0
                                    onImplicitWidthChanged: widest = Math.max(widest, implicitWidth)
                                    Layout.preferredWidth: widest
                                    spacing: 0
                                    QQC2.Label {
                                        text: figure.pair[0]
                                        color: Tokens.textSecondary
                                        font: Tokens.caption
                                    }
                                    QQC2.Label {
                                        text: figure.pair[1]
                                        color: Tokens.textPrimary
                                        font: Tokens.subtitle
                                    }
                                }
                            }
                        }

                        GridLayout {
                            Layout.alignment: Qt.AlignTop
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: Tokens.s6
                            rowSpacing: Tokens.s2
                            Repeater {
                                model: page.facts.length * 2
                                delegate: QQC2.Label {
                                    required property int index
                                    readonly property bool isValue: index % 2 === 1
                                    Layout.fillWidth: isValue
                                    text: page.facts[Math.floor(index / 2)][index % 2]
                                    color: isValue ? Tokens.textPrimary : Tokens.textSecondary
                                    font: Tokens.body
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    readonly property var bigFigures: {
        if (current === "cpu")
            return [[i18nc("@label", "Utilization"), Fmt.percent(performance.cpuPercent)],
                    [i18nc("@label the processor's clock now", "Speed"), Fmt.ghz(performance.cpuMhz || cpuInfo.maxMhz)],
                    [i18nc("@label", "Processes"), String(processCount)],
                    [i18nc("@label", "Threads"), String(performance.threadCount)],
                    [i18nc("@label time since the machine started", "Up time"), Fmt.uptime(performance.uptimeSeconds)]]
        if (current === "memory")
            return [[i18nc("@label memory", "In use"), Fmt.bytes(mem.inUse)],
                    [i18nc("@label memory", "Available"), Fmt.bytes(mem.available)],
                    [i18nc("@label memory promised to programs, and the most that can be", "Committed"),
                     i18nc("@info committed of the limit", "%1/%2", Fmt.bytes(mem.committed), Fmt.bytes(mem.commitLimit))],
                    [i18nc("@label memory holding files", "Cached"), Fmt.bytes(mem.cached)]]
        if (disk)
            return [[i18nc("@label", "Active time"), Fmt.percent(disk.active)],
                    [i18nc("@label disk", "Read speed"), Fmt.rate(disk.read)],
                    [i18nc("@label disk", "Write speed"), Fmt.rate(disk.write)]]
        if (adapter)
            return [[i18nc("@label network", "Send"), Fmt.bits(adapter.sent)],
                    [i18nc("@label network", "Receive"), Fmt.bits(adapter.received)]]
        if (gpu)
            return gpu.busy >= 0 ? [[i18nc("@label", "Utilization"), Fmt.percent(gpu.busy)]] : []
        return []
    }

    readonly property var facts: {
        const yes = i18nc("@info", "Yes"), no = i18nc("@info", "No")
        if (current === "cpu")
            return [[i18nc("@label the processor's highest clock", "Base speed:"), cpuInfo.maxMhz > 0 ? Fmt.ghz(cpuInfo.maxMhz) : "—"],
                    [i18nc("@label", "Sockets:"), String(cpuInfo.sockets)],
                    [i18nc("@label", "Cores:"), String(cpuInfo.cores)],
                    [i18nc("@label", "Logical processors:"), String(cpuInfo.logical)],
                    [i18nc("@label running inside a virtual machine", "Virtual machine:"), cpuInfo.virtualMachine ? yes : no],
                    [i18nc("@label", "L1 cache:"), Fmt.bytes(cpuInfo.l1)],
                    [i18nc("@label", "L2 cache:"), Fmt.bytes(cpuInfo.l2)],
                    [i18nc("@label", "L3 cache:"), cpuInfo.l3 > 0 ? Fmt.bytes(cpuInfo.l3) : "—"]]
        if (current === "memory")
            return [[i18nc("@label memory", "Total:"), Fmt.bytes(mem.total)],
                    [i18nc("@label the swap space", "Swap:"), mem.swapTotal > 0
                        ? i18nc("@info used of total", "%1 of %2", Fmt.bytes(mem.swapUsed), Fmt.bytes(mem.swapTotal)) : i18nc("@info no swap", "None")]]
        if (disk)
            return [[i18nc("@label", "Capacity:"), Fmt.capacity(disk.capacity)],
                    [i18nc("@label", "Type:"), diskKind(disk)],
                    [i18nc("@label the disk's name in the system", "Device:"), "/dev/" + disk.name]]
        if (adapter)
            return [[i18nc("@label", "Adapter name:"), adapter.name],
                    [i18nc("@label", "Connection type:"), adapterTitle(adapter)],
                    [i18nc("@label", "Link speed:"), adapter.linkMbps > 0 ? Fmt.bits(adapter.linkMbps * 1e6) : "—"],
                    [i18nc("@label", "IPv4 address:"), adapter.ipv4 || "—"],
                    [i18nc("@label", "IPv6 address:"), adapter.ipv6 || "—"]]
        if (gpu)
            return [[i18nc("@label", "Driver:"), gpu.driver],
                    [i18nc("@label", "Dedicated memory:"), gpu.vramTotal > 0
                        ? i18nc("@info used of total", "%1 of %2", Fmt.bytes(gpu.vramUsed), Fmt.bytes(gpu.vramTotal)) : "—"]]
        return []
    }

    // a device in the list on the left
    component Tile: QQC2.AbstractButton {
        id: tile
        property string key
        property string title
        property string detail
        property var values: []
        property real maximum: 100
        property color tint: Tokens.accent

        Layout.fillWidth: true
        implicitHeight: 64
        hoverEnabled: true
        Accessible.name: title
        onClicked: page.current = key

        background: Rectangle {
            radius: Tokens.radiusControl
            color: page.current === tile.key ? Tokens.fillSubtleSelected : tile.hovered ? Tokens.fillSubtleHover : "transparent"
            Rectangle {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: Tokens.sizePillWidth
                height: page.current === tile.key ? Tokens.sizePillHeight : 0
                radius: Tokens.radiusNavPill
                color: Tokens.accent
            }
        }
        contentItem: RowLayout {
            spacing: Tokens.s4
            Graph {
                Layout.preferredWidth: 72
                Layout.preferredHeight: 48
                grid: false
                values: tile.values
                maximum: tile.maximum
                color: tile.tint
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                QQC2.Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: tile.title
                    color: Tokens.textPrimary
                    font: Tokens.bodyStrong
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    maximumLineCount: 2
                    text: tile.detail
                    color: Tokens.textSecondary
                    font: Tokens.caption
                }
            }
        }
        leftPadding: Tokens.s3
        rightPadding: Tokens.s2
    }
}

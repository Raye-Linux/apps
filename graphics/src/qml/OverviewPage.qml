// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Overview: the card's readings, live, with the last minute of each, and
// what the card is. A reading the card does not give is not shown.

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

QQC2.ScrollView {
    id: page

    required property var model
    readonly property var now: model.now
    readonly property var history: model.history
    readonly property var card: model.card
    readonly property bool nvidia: card.vendor === "nvidia"
    readonly property bool amd: card.vendor === "amd"
    readonly property bool hasReadings: amd || (nvidia && model.nvidiaReadings)

    contentWidth: availableWidth
    QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff

    function seen(v) { return v !== undefined && v >= 0 }
    function peak(list, floor) {
        let m = floor
        for (const v of list || [])
            m = Math.max(m, v)
        return m
    }

    WheelScroll { target: page.contentItem }

    ColumnLayout {
        width: page.availableWidth
        spacing: Tokens.s3

        QQC2.Label {
            Layout.fillWidth: true
            text: page.card.model || i18nc("@title", "Graphics card")
            color: Tokens.textPrimary
            font: Tokens.subtitle
            elide: Text.ElideRight
        }
        QQC2.Label {
            Layout.fillWidth: true
            Layout.bottomMargin: Tokens.s3
            visible: page.amd || page.nvidia
            text: page.nvidia ? i18nc("@info", "NVIDIA graphics") : i18nc("@info", "AMD Radeon graphics")
            color: Tokens.textSecondary
            font: Tokens.body
        }

        // --- NVIDIA: its own panel, and readings once its driver is in ---
        SettingsCard {
            Layout.fillWidth: true
            // a fixed hint: with a long wrapped description the card's own
            // width hint and its text keep resizing each other
            implicitWidth: 320
            visible: page.nvidia
            icon.name: page.model.nvidiaPanel ? "nvidia-settings" : ""
            title: i18nc("@label", "NVIDIA Control Panel")
            description: page.model.nvidiaPanel
                         ? i18nc("@info", "Change this card's settings: performance, displays, color and more.")
                         : i18nc("@info", "It comes with the NVIDIA driver, which Raye installs when the PC is online. Check Settings > Raye Update > Optional updates.")
            AccentButton {
                visible: page.model.nvidiaPanel
                text: i18nc("@action:button", "Open")
                onClicked: page.model.openNvidiaPanel()
            }
        }
        InfoBar {
            Layout.fillWidth: true
            visible: page.nvidia && !page.model.nvidiaReadings
            closable: false
            message: i18nc("@info", "The readings of this card show up here once the NVIDIA driver is installed.")
        }

        // --- a card this panel cannot read ---------------------------------
        InfoBar {
            Layout.fillWidth: true
            visible: !page.amd && !page.nvidia
            closable: false
            title: page.card.vendor === undefined ? i18nc("@info", "No graphics card was found.")
                 : page.card.vendor === "intel" ? i18nc("@info", "This PC has Intel graphics.")
                 : i18nc("@info", "This graphics card isn't one this panel knows.")
            message: page.card.vendor === undefined ? ""
                   : i18nc("@info", "This panel shows and changes the settings of AMD Radeon cards, and shows NVIDIA cards. Your card sets itself up: there is nothing to change here. Display settings are in Settings > Display.")
        }
        InfoBar {
            Layout.fillWidth: true
            visible: page.amd && page.card.driver !== "amdgpu"
            closable: false
            message: i18nc("@info", "This card runs on an older driver, so its settings can't be changed here.")
        }

        // --- the readings ------------------------------------------------------
        GridLayout {
            Layout.fillWidth: true
            visible: page.hasReadings
            columns: page.availableWidth >= 900 ? 3 : 2
            columnSpacing: Tokens.s3
            rowSpacing: Tokens.s3
            uniformCellWidths: true

            Tile {
                Layout.fillWidth: true
                visible: page.seen(page.now.busy)
                title: i18nc("@label graphics card use", "GPU")
                value: Fmt.percent(page.now.busy ?? 0)
                detail: page.seen(page.now.memBusy) ? i18nc("@info", "Memory controller %1", Fmt.percent(page.now.memBusy)) : ""
                values: page.history.busy || []
            }
            Tile {
                Layout.fillWidth: true
                visible: page.seen(page.now.sclk)
                title: i18nc("@label", "Clock speed")
                value: Fmt.mhz(page.now.sclk ?? 0)
                detail: page.seen(page.now.mclk) ? i18nc("@info", "Memory %1", Fmt.mhz(page.now.mclk)) : ""
                values: page.history.sclk || []
                maximum: page.peak(page.history.sclk, 100)
                color: Tokens.statusInfo
            }
            Tile {
                Layout.fillWidth: true
                visible: page.now.vramTotal > 0
                title: page.now.vramTotal >= 1073741824 ? i18nc("@label", "Video memory") : i18nc("@label", "Reserved video memory")
                value: Fmt.bytes(page.now.vramUsed ?? 0)
                detail: i18nc("@info used of total", "of %1", Fmt.bytes(page.now.vramTotal ?? 0))
                values: page.history.vram || []
                color: Tokens.brandMagenta
            }
            Tile {
                Layout.fillWidth: true
                // an integrated GPU works mostly in the PC's own memory
                visible: page.now.gttTotal > 0 && page.now.vramTotal < 1073741824
                title: i18nc("@label", "Shared memory")
                value: Fmt.bytes(page.now.gttUsed ?? 0)
                detail: i18nc("@info used of total", "of %1", Fmt.bytes(page.now.gttTotal ?? 0))
                values: []
                color: Tokens.brandMagenta
            }
            Tile {
                Layout.fillWidth: true
                visible: page.seen(page.now.temp)
                title: i18nc("@label", "Temperature")
                value: Fmt.celsius(page.now.temp ?? 0)
                detail: [page.seen(page.now.junction) ? i18nc("@info hottest point of the chip", "Hotspot %1", Fmt.celsius(page.now.junction)) : "",
                         page.seen(page.now.memTemp) ? i18nc("@info", "Memory %1", Fmt.celsius(page.now.memTemp)) : ""]
                        .filter(s => s !== "").join("  ·  ")
                values: page.history.temp || []
                maximum: 110
                color: Tokens.statusCaution
            }
            Tile {
                Layout.fillWidth: true
                visible: page.seen(page.now.powerW)
                title: i18nc("@label", "Power")
                value: Fmt.watts(page.now.powerW ?? 0)
                detail: page.now.powerLimitW > 0 ? i18nc("@info", "Limit %1", Fmt.watts(page.now.powerLimitW)) : ""
                values: page.history.power || []
                maximum: Math.max(page.now.powerLimitW || 0, page.peak(page.history.power, 10))
                color: Tokens.statusSuccess
            }
            Tile {
                Layout.fillWidth: true
                visible: page.seen(page.now.fanPercent) || page.seen(page.now.fanRpm)
                title: i18nc("@label", "Fan")
                value: page.seen(page.now.fanPercent) ? Fmt.percent(page.now.fanPercent) : Fmt.rpm(page.now.fanRpm)
                detail: page.seen(page.now.fanRpm) && page.seen(page.now.fanPercent)
                        ? (page.now.fanRpm === 0 ? i18nc("@info", "Stopped while cool") : Fmt.rpm(page.now.fanRpm)) : ""
                values: page.history.fan || []
                color: Tokens.textSecondary
            }
        }

        // --- what the card is -------------------------------------------------
        Card {
            Layout.fillWidth: true
            Layout.topMargin: Tokens.s3
            visible: page.card.slot !== undefined

            GridLayout {
                id: facts
                anchors.fill: parent
                columns: 2
                columnSpacing: Tokens.s8
                rowSpacing: Tokens.s2

                component Key: QQC2.Label { color: Tokens.textSecondary; font: Tokens.body }
                component Value: QQC2.Label {
                    Layout.fillWidth: true
                    color: Tokens.textPrimary
                    font: Tokens.body
                    elide: Text.ElideRight
                }

                Key { text: i18nc("@label", "Driver") }
                Value { text: [page.card.driver, page.nvidia && page.now.driver ? page.now.driver : ""].filter(s => s).join(" ") }
                Key { text: i18nc("@label", "Location"); visible: !!page.card.slot }
                Value { text: i18nc("@info PCI address", "PCI %1", page.card.slot); visible: !!page.card.slot }
                Key { text: i18nc("@label card firmware", "Firmware"); visible: !!page.card.vbios }
                Value { text: page.card.vbios || ""; visible: !!page.card.vbios }
            }
        }
    }
}

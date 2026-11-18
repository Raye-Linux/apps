// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The stopwatch: the time in very big digits, Start/Pause, Lap and Reset
// under it, and the laps below, the fastest and slowest marked. It keeps
// going with the window closed.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

Item {
    id: page

    property var clock
    property bool shown: true
    property double nowMs: Date.now()
    readonly property double elapsed: clock.stopwatchRunning ? clock.stopwatchBanked + Math.max(0, nowMs - clock.stopwatchSince)
                                                             : clock.stopwatchBanked
    readonly property var parts: {
        const t = Math.floor(elapsed / 10)
        return {
            h: Math.floor(t / 360000),
            m: Math.floor(t / 6000) % 60,
            s: Math.floor(t / 100) % 60,
            c: t % 100,
        }
    }
    function pad(n) { return n < 10 ? "0" + n : "" + n }

    // every frame while it runs and shows: the hundredths move smoothly
    FrameAnimation {
        running: page.shown && page.clock.stopwatchRunning
        onTriggered: page.nowMs = Date.now()
        onRunningChanged: page.nowMs = Date.now()
    }

    Keys.onSpacePressed: page.clock.stopwatchRunning ? page.clock.pauseStopwatch() : page.clock.startStopwatch()
    Component.onCompleted: forceActiveFocus()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Tokens.s8
        spacing: Tokens.s6

        Item { Layout.preferredHeight: Math.max(0, (page.height - 640) / 4) }

        // the digits, each group with its unit under it, as Windows draws them
        Row {
            id: digits
            Layout.alignment: Qt.AlignHCenter
            readonly property int size: Math.max(56, Math.min(112, page.width / 9))
            spacing: 0
            Repeater {
                model: [
                    { text: page.pad(page.parts.h), unit: i18nc("@label short for hours", "hr") },
                    { text: ":" },
                    { text: page.pad(page.parts.m), unit: i18nc("@label short for minutes", "min") },
                    { text: ":" },
                    { text: page.pad(page.parts.s), unit: i18nc("@label short for seconds", "sec") },
                    { text: "." },
                    { text: page.pad(page.parts.c), unit: "" },
                ]
                delegate: Column {
                    required property var modelData
                    Text {
                        text: modelData.text
                        color: modelData.unit === undefined ? Tokens.textSecondary : Tokens.textPrimary
                        font.family: "Inter Display"
                        font.pixelSize: digits.size
                        font.weight: modelData.unit === undefined ? Font.Normal : Font.DemiBold
                        font.features: { "tnum": 1 }
                    }
                    QQC2.Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: modelData.unit ?? ""
                        color: Tokens.textSecondary
                        font: Tokens.body
                    }
                }
            }
        }

        Row {
            Layout.alignment: Qt.AlignHCenter
            spacing: Tokens.s5
            RoundButton {
                size: 56
                glyph: page.clock.stopwatchRunning ? "pause" : "play"
                accent: true
                tip: page.clock.stopwatchRunning ? i18nc("@action:button", "Pause") : i18nc("@action:button", "Start")
                onClicked: page.clock.stopwatchRunning ? page.clock.pauseStopwatch() : page.clock.startStopwatch()
            }
            RoundButton {
                size: 56
                glyph: "flag"
                tip: i18nc("@action:button stopwatch", "Lap")
                enabled: page.clock.stopwatchRunning
                onClicked: page.clock.lap()
            }
            RoundButton {
                size: 56
                glyph: "reset"
                tip: i18nc("@action:button", "Reset")
                enabled: page.elapsed > 0
                onClicked: page.clock.resetStopwatch()
            }
        }

        // the laps, newest on top
        Card {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Math.min(640, page.width - 2 * Tokens.s8)
            Layout.fillHeight: true
            Layout.minimumHeight: 120
            padding: 0
            opacity: page.clock.laps.length > 0 ? 1 : 0
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: Tokens.durationNormal } }

            contentItem: ColumnLayout {
                spacing: 0
                RowLayout {
                    Layout.fillWidth: true
                    Layout.margins: Tokens.s5
                    Layout.bottomMargin: Tokens.s3
                    QQC2.Label {
                        Layout.preferredWidth: 120
                        text: i18nc("@title:column", "Laps")
                        color: Tokens.textSecondary
                        font: Tokens.caption
                    }
                    QQC2.Label {
                        Layout.fillWidth: true
                        text: i18nc("@title:column", "Time")
                        color: Tokens.textSecondary
                        font: Tokens.caption
                    }
                    QQC2.Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignRight
                        text: i18nc("@title:column", "Total")
                        color: Tokens.textSecondary
                        font: Tokens.caption
                    }
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Tokens.strokeDivider
                }
                ListView {
                    id: lapList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: page.clock.laps
                    boundsBehavior: Flickable.StopAtBounds
                    QQC2.ScrollBar.vertical: Scroller {}
                    WheelScroll { target: lapList }
                    delegate: Item {
                        id: row
                        required property var modelData
                        required property int index
                        width: lapList.width
                        height: Tokens.sizeListItem
                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: Tokens.s1
                            radius: Tokens.radiusControl
                            color: row.index === 0 ? Tokens.fillSubtleHover : "transparent"
                        }
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Tokens.s5
                            anchors.rightMargin: Tokens.s5
                            RowLayout {
                                Layout.preferredWidth: 120
                                spacing: Tokens.s3
                                QQC2.Label {
                                    text: row.modelData.number
                                    color: Tokens.textPrimary
                                    font: Tokens.body
                                }
                                QQC2.Label {
                                    visible: row.modelData.fastest || row.modelData.slowest
                                    text: row.modelData.fastest ? i18nc("@info lap", "Fastest") : i18nc("@info lap", "Slowest")
                                    color: row.modelData.fastest ? Tokens.statusSuccess : Tokens.statusCritical
                                    font: Tokens.caption
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: row.modelData.lap
                                color: Tokens.textPrimary
                                font.family: "Inter"
                                font.pixelSize: 14
                                font.features: { "tnum": 1 }
                            }
                            Text {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignRight
                                text: row.modelData.total
                                color: Tokens.textSecondary
                                font.family: "Inter"
                                font.pixelSize: 14
                                font.features: { "tnum": 1 }
                            }
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
            visible: page.clock.laps.length === 0
        }
    }
}

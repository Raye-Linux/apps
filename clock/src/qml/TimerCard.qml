// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// One timer: its name, the ring of what is left with the time in big
// digits, and Start/Pause and Reset under it. While editing, a click
// opens it and the bin removes it.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

Card {
    id: card

    property var clock
    property string title
    property double durationMs
    property double remainingMs
    property double endsAt
    property bool running
    property bool editing

    signal toggle()
    signal reset()
    signal remove()
    signal edit()

    readonly property bool over: running && remainingMs <= 0
    readonly property bool touched: running || remainingMs < durationMs

    hoverable: editing
    padding: Tokens.s6
    Accessible.name: title

    TapHandler {
        enabled: card.editing
        onTapped: card.edit()
    }

    contentItem: ColumnLayout {
        spacing: Tokens.s4

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: Tokens.sizeControlHeight
            QQC2.Label {
                Layout.fillWidth: true
                text: card.title
                color: Tokens.textPrimary
                font: Tokens.bodyStrong
                elide: Text.ElideRight
            }
            FlatButton {
                visible: card.editing
                glyph: "delete"
                tip: i18nc("@action:button", "Delete timer")
                onClicked: card.remove()
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ProgressRing {
                id: ring
                anchors.centerIn: parent
                width: Math.min(parent.width, parent.height, 224)
                height: width
                thickness: 7
                value: card.durationMs > 0 ? card.remainingMs / card.durationMs : 0
                color: card.over ? Tokens.statusCaution : Tokens.accent
                opacity: card.touched ? 1 : 0.85
            }

            Column {
                anchors.centerIn: ring
                spacing: Tokens.s1
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: card.clock.clockText(card.remainingMs)
                    color: card.over ? Tokens.statusCaution : Tokens.textPrimary
                    font.family: "Inter Display"
                    font.pixelSize: Math.round(ring.width * 0.18)
                    font.weight: Font.DemiBold
                    font.features: { "tnum": 1 }
                }
                QQC2.Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    // when it rings, for a running timer
                    text: card.over ? i18nc("@info", "Time's up")
                        : card.running ? i18nc("@info time a timer ends", "Ends at %1",
                                               new Date(card.endsAt).toLocaleTimeString(Qt.locale(), Locale.ShortFormat))
                        : card.touched ? i18nc("@info", "Paused") : " "
                    color: Tokens.textSecondary
                    font: Tokens.caption
                }
            }
        }

        Row {
            Layout.alignment: Qt.AlignHCenter
            spacing: Tokens.s4
            opacity: card.editing ? 0.35 : 1
            enabled: !card.editing
            Behavior on opacity { NumberAnimation { duration: Tokens.durationFast } }
            RoundButton {
                glyph: card.running ? "pause" : "play"
                accent: true
                tip: card.running ? i18nc("@action:button", "Pause") : i18nc("@action:button", "Start")
                onClicked: card.toggle()
            }
            RoundButton {
                glyph: "reset"
                tip: i18nc("@action:button", "Reset")
                enabled: card.touched
                onClicked: card.reset()
            }
        }
    }
}

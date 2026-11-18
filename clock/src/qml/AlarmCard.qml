// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// One alarm: its time in big digits with the switch beside it, when it
// rings next, its name and its days. Off, it fades back. A click opens it.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

Card {
    id: card

    property var model
    property string name
    property string clockText
    property string meridiem
    property int days
    property bool on
    property string ringsIn
    property bool editing

    signal switched(bool on)
    signal remove()
    signal edit()

    hoverable: true
    padding: Tokens.s6
    Accessible.name: (name !== "" ? name + ", " : "") + clockText + " " + meridiem

    TapHandler {
        onTapped: card.edit()
    }

    contentItem: ColumnLayout {
        spacing: Tokens.s2

        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.s2
            Text {
                text: card.clockText
                color: card.on ? Tokens.textPrimary : Tokens.textTertiary
                font.family: "Inter Display"
                font.pixelSize: 52
                font.weight: Font.DemiBold
                font.features: { "tnum": 1 }
                Behavior on color { ColorAnimation { duration: Tokens.durationFast } }
            }
            Text {
                Layout.alignment: Qt.AlignBaseline
                visible: card.meridiem !== ""
                text: card.meridiem
                color: card.on ? Tokens.textSecondary : Tokens.textTertiary
                font.family: "Inter Display"
                font.pixelSize: 20
                font.weight: Font.Medium
            }
            Item { Layout.fillWidth: true }
            FlatButton {
                Layout.alignment: Qt.AlignTop
                visible: card.editing
                glyph: "delete"
                tip: i18nc("@action:button", "Delete alarm")
                onClicked: card.remove()
            }
            Switch {
                Layout.alignment: Qt.AlignTop
                visible: !card.editing
                showStateText: false
                text: i18nc("@info:tooltip", "Alarm on")
                checked: card.on
                onToggled: card.switched(checked)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.s3
            opacity: card.on ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: Tokens.durationFast } }
            Glyph {
                kind: "bell"
                color: Tokens.textSecondary
            }
            QQC2.Label {
                Layout.fillWidth: true
                text: card.ringsIn
                color: Tokens.textSecondary
                font: Tokens.body
                elide: Text.ElideRight
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            text: card.name !== "" ? card.name : i18nc("@info alarm with no name", "Alarm")
            color: card.on ? Tokens.textPrimary : Tokens.textTertiary
            font: Tokens.bodyStrong
            elide: Text.ElideRight
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            DayChips {
                visible: card.days !== 0
                clock: card.model
                days: card.days
                editable: false
                size: 28
                opacity: card.on ? 1 : 0.5
            }
            QQC2.Label {
                visible: card.days === 0
                text: i18nc("@info alarm that rings once", "Only once")
                color: Tokens.textTertiary
                font: Tokens.caption
            }
        }
    }
}

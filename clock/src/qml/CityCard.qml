// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// One city of the world clock: a soft wash of day or night behind it,
// the city and its country, the time there, and how far it is from here.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

Card {
    id: card

    property string name
    property string country
    property string clockText
    property string meridiem
    property string when
    property bool daytime
    property bool editing
    signal remove()

    padding: Tokens.s6
    Accessible.name: name + ", " + clockText + " " + meridiem + ", " + when

    // day: a warm glow from the top right; night: the Raye violet, deeper
    Rectangle {
        parent: card.background
        anchors.fill: parent
        anchors.margins: 1
        radius: Tokens.radiusOverlay - 1
        opacity: Tokens.night ? 0.55 : 0.4
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0; color: "transparent" }
            GradientStop { position: 1; color: card.daytime ? Qt.alpha("#ffb347", Tokens.night ? 0.22 : 0.3)
                                                           : Qt.alpha(Tokens.brandViolet, Tokens.night ? 0.35 : 0.22) }
        }
    }

    contentItem: ColumnLayout {
        spacing: Tokens.s1
        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                QQC2.Label {
                    Layout.fillWidth: true
                    text: card.name
                    color: Tokens.textPrimary
                    font: Tokens.subtitle
                    elide: Text.ElideRight
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    visible: card.country !== ""
                    text: card.country
                    color: Tokens.textSecondary
                    font: Tokens.caption
                    elide: Text.ElideRight
                }
            }
            FlatButton {
                visible: card.editing
                glyph: "delete"
                tip: i18nc("@action:button", "Remove city")
                onClicked: card.remove()
            }
            Glyph {
                visible: !card.editing
                Layout.preferredWidth: 20
                Layout.preferredHeight: 20
                kind: card.daytime ? "sun" : "moon"
                color: card.daytime ? Tokens.statusCaution : Tokens.accentText
            }
        }
        Item { Layout.fillHeight: true }
        RowLayout {
            spacing: Tokens.s2
            Text {
                text: card.clockText
                color: Tokens.textPrimary
                font.family: "Inter Display"
                font.pixelSize: 40
                font.weight: Font.DemiBold
                font.features: { "tnum": 1 }
            }
            Text {
                Layout.alignment: Qt.AlignBaseline
                visible: card.meridiem !== ""
                text: card.meridiem
                color: Tokens.textSecondary
                font.family: "Inter Display"
                font.pixelSize: 16
            }
        }
        QQC2.Label {
            Layout.fillWidth: true
            text: card.when
            color: Tokens.textSecondary
            font: Tokens.body
            elide: Text.ElideRight
        }
    }
}

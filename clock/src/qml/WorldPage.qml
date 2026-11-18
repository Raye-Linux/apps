// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// World clock: the time here in big digits, then a card for each city
// chosen, with how far ahead or behind it is and whether it is day there.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

Item {
    id: page

    property var clock
    property date now: new Date()
    property bool editing: false

    GridView {
        id: grid
        anchors.fill: parent
        anchors.leftMargin: Tokens.s8
        anchors.topMargin: Tokens.s8
        clip: true
        readonly property int columns: Math.max(1, Math.floor((width - Tokens.s8 + Tokens.s5) / (300 + Tokens.s5)))
        cellWidth: Math.floor((width - Tokens.s8) / columns)
        cellHeight: 168 + Tokens.s5
        model: page.clock.cities
        boundsBehavior: Flickable.StopAtBounds
        footer: Item { width: 1; height: 96 }
        QQC2.ScrollBar.vertical: Scroller {}
        WheelScroll { target: grid }

        // the time here, across the top
        header: Item {
            // as wide as the cards' row, gaps left out
            width: grid.width - Tokens.s8 - Tokens.s5
            height: here.implicitHeight + Tokens.s5
            Card {
                id: here
                width: parent.width
                padding: Tokens.s7
                contentItem: RowLayout {
                    spacing: Tokens.s7
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Tokens.s1
                        QQC2.Label {
                            text: i18nc("@title", "Local time")
                            color: Tokens.textSecondary
                            font: Tokens.bodyStrong
                        }
                        RowLayout {
                            readonly property var parts: page.clock.timeParts(page.now, true)
                            spacing: Tokens.s3
                            Text {
                                text: parent.parts.clock
                                color: Tokens.textPrimary
                                font.family: "Inter Display"
                                font.pixelSize: 64
                                font.weight: Font.DemiBold
                                font.features: { "tnum": 1 }
                            }
                            Text {
                                Layout.alignment: Qt.AlignBaseline
                                visible: text !== ""
                                text: parent.parts.meridiem
                                color: Tokens.textSecondary
                                font.family: "Inter Display"
                                font.pixelSize: 24
                            }
                        }
                        QQC2.Label {
                            text: page.now.toLocaleDateString(Qt.locale(), i18nc("@info date format, QDate pattern", "dddd, MMMM d, yyyy"))
                            color: Tokens.textSecondary
                            font: Tokens.bodyLarge
                        }
                    }
                    ColumnLayout {
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                        spacing: Tokens.s2
                        Glyph {
                            Layout.alignment: Qt.AlignRight
                            Layout.preferredWidth: 40
                            Layout.preferredHeight: 40
                            stroke: 1
                            kind: page.now.getHours() >= 6 && page.now.getHours() < 19 ? "sun" : "moon"
                            color: Tokens.accentText
                        }
                        QQC2.Label {
                            Layout.alignment: Qt.AlignRight
                            text: page.clock.localPlace
                            color: Tokens.textSecondary
                            font: Tokens.body
                        }
                    }
                }
            }
        }

        add: Transition {
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Tokens.durationSlow; easing.type: Tokens.easingSlow }
                NumberAnimation { property: "scale"; from: 0.92; to: 1; duration: Tokens.durationSlow; easing.type: Tokens.easingSlow }
            }
        }
        remove: Transition {
            NumberAnimation { property: "opacity"; to: 0; duration: Tokens.durationExit; easing.type: Tokens.easingExit }
        }
        displaced: Transition {
            NumberAnimation { properties: "x,y"; duration: Tokens.durationNormal; easing.type: Tokens.easingNormal }
        }

        delegate: Item {
            id: cell
            required property string key
            required property string name
            required property string country
            required property string clock
            required property string meridiem
            required property string dayText
            required property string gapText
            required property bool daytime
            width: grid.cellWidth
            height: grid.cellHeight
            CityCard {
                anchors.fill: parent
                anchors.rightMargin: Tokens.s5
                anchors.bottomMargin: Tokens.s5
                name: cell.name
                country: cell.country
                clockText: cell.clock
                meridiem: cell.meridiem
                when: i18nc("@info day, then how far: Today, 6 hours ahead", "%1, %2", cell.dayText, cell.gapText)
                daytime: cell.daytime
                editing: page.editing
                onRemove: page.clock.removeCity(cell.key)
            }
        }
    }

    QQC2.Label {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Tokens.s10 * 2
        visible: page.clock.cities.count === 0
        text: i18nc("@info", "Select + to see the time in another city.")
        color: Tokens.textSecondary
        font: Tokens.body
    }

    Row {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Tokens.s7
        spacing: Tokens.s3
        RoundButton {
            glyph: page.editing ? "check" : "edit"
            tip: page.editing ? i18nc("@action:button", "Done") : i18nc("@action:button", "Edit cities")
            visible: page.clock.cities.count > 0
            onClicked: page.editing = !page.editing
        }
        RoundButton {
            glyph: "add"
            accent: true
            tip: i18nc("@action:button", "Add a city")
            onClicked: sheet.openSearch()
        }
    }

    CitySheet {
        id: sheet
        clock: page.clock
    }
}

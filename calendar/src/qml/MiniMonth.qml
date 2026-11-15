// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The small month of the side pane and of the date picker: arrows, the
// days, today in the accent, the chosen day ringed, a dot under days that
// hold something.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui
import "datelogic.js" as D

ColumnLayout {
    id: mini

    property date selected: new Date()
    property date today: new Date()
    property int shownYear: selected.getFullYear()
    property int shownMonth: selected.getMonth()
    // "yyyy-MM-dd" -> true for the days to mark
    property var marked: ({})
    readonly property var cells: D.monthCells(shownYear, shownMonth, Qt.locale().firstDayOfWeek)
    signal picked(date day)

    function move(delta) {
        const d = new Date(shownYear, shownMonth + delta, 1)
        shownYear = d.getFullYear()
        shownMonth = d.getMonth()
    }

    onSelectedChanged: {
        shownYear = selected.getFullYear()
        shownMonth = selected.getMonth()
    }

    spacing: Tokens.s2

    RowLayout {
        Layout.fillWidth: true
        QQC2.Label {
            Layout.fillWidth: true
            Layout.leftMargin: Tokens.s2
            text: new Date(mini.shownYear, mini.shownMonth, 1).toLocaleDateString(Qt.locale(), "MMMM yyyy")
            color: Tokens.textPrimary
            font: Tokens.bodyStrong
        }
        FlatButton {
            glyph: "back"
            tip: i18nc("@info:tooltip", "Previous month")
            onClicked: mini.move(-1)
        }
        FlatButton {
            glyph: "forward"
            tip: i18nc("@info:tooltip", "Next month")
            onClicked: mini.move(1)
        }
    }

    Grid {
        columns: 7
        Repeater {
            model: 7
            QQC2.Label {
                required property int index
                width: 36
                height: 24
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: Qt.locale().dayName((Qt.locale().firstDayOfWeek + index) % 7, Locale.ShortFormat).substring(0, 2)
                color: Tokens.textSecondary
                font: Tokens.caption
            }
        }
        Repeater {
            model: mini.cells
            QQC2.AbstractButton {
                id: cell
                required property var modelData
                readonly property bool isToday: D.sameDay(modelData, mini.today)
                readonly property bool isSelected: D.sameDay(modelData, mini.selected)
                readonly property bool inMonth: modelData.getMonth() === mini.shownMonth
                width: 36
                height: 34
                hoverEnabled: true
                Accessible.name: modelData.toLocaleDateString(Qt.locale(), Locale.LongFormat)
                onClicked: mini.picked(modelData)
                background: Item {
                    Rectangle {
                        anchors.centerIn: parent
                        width: 30
                        height: 30
                        radius: 15
                        color: cell.isToday ? Tokens.accent : cell.hovered ? Tokens.fillSubtleHover : "transparent"
                        border.width: cell.isSelected && !cell.isToday ? 1.5 : 0
                        border.color: Tokens.accent
                    }
                }
                contentItem: Item {
                    Text {
                        anchors.centerIn: parent
                        anchors.verticalCenterOffset: -1
                        text: cell.modelData.getDate()
                        color: cell.isToday ? Tokens.textOnAccent : cell.inMonth ? Tokens.textPrimary : Tokens.textDisabled
                        font.family: "Inter"
                        font.pixelSize: 12
                        font.weight: cell.isToday ? Font.DemiBold : Font.Normal
                        font.features: { "tnum": 1 }
                    }
                    Rectangle {
                        visible: mini.marked[D.isoDay(cell.modelData)] === true
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 5
                        width: 4
                        height: 4
                        radius: 2
                        color: cell.isToday ? Tokens.textOnAccent : Tokens.accentText
                    }
                }
            }
        }
    }
}

// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The month: six weeks of seven days, each day with what fits of its
// events and "+2 more" for the rest. A click picks the day, a double
// click starts a new event on it.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui
import "datelogic.js" as D

Item {
    id: view

    property var model
    property date focusDay: new Date()
    property date today: new Date()
    readonly property var cells: D.monthCells(focusDay.getFullYear(), focusDay.getMonth(), Qt.locale().firstDayOfWeek)
    readonly property var occ: model.revision >= 0 ? model.occurrences(cells[0], D.addDays(cells[41], 1)) : []

    signal dayPicked(date day)
    signal dayOpened(date day)
    signal newEvent(date day)
    signal editOccurrence(var occ)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Row {
            Layout.fillWidth: true
            Repeater {
                model: 7
                QQC2.Label {
                    required property int index
                    width: view.width / 7
                    height: 28
                    leftPadding: Tokens.s3
                    verticalAlignment: Text.AlignVCenter
                    text: Qt.locale().dayName((Qt.locale().firstDayOfWeek + index) % 7, Locale.ShortFormat)
                    color: Tokens.textSecondary
                    font: Tokens.caption
                }
            }
        }

        Grid {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 7
            readonly property real cellW: width / 7
            readonly property real cellH: height / 6

            Repeater {
                model: view.cells
                Rectangle {
                    id: cell
                    required property var modelData
                    required property int index
                    readonly property bool inMonth: modelData.getMonth() === view.focusDay.getMonth()
                    readonly property bool isToday: D.sameDay(modelData, view.today)
                    readonly property bool isFocus: D.sameDay(modelData, view.focusDay)
                    readonly property var dayOcc: D.onDay(view.occ, modelData)
                    readonly property int fits: Math.max(0, Math.floor((height - 30 - 18) / 24))
                    width: grid.cellW
                    height: grid.cellH
                    color: isFocus ? Tokens.fillSubtleSelected : dayHover.hovered ? Tokens.fillSubtleHover
                         : inMonth ? "transparent" : Qt.alpha(Tokens.bgSecondary, 0.6)
                    border.width: 0

                    Rectangle {
                        anchors.right: parent.right
                        width: 1
                        height: parent.height
                        color: Tokens.strokeDivider
                        visible: cell.index % 7 !== 6
                    }
                    Rectangle {
                        anchors.top: parent.top
                        width: parent.width
                        height: 1
                        color: Tokens.strokeDivider
                    }

                    HoverHandler { id: dayHover }
                    TapHandler {
                        onTapped: view.dayPicked(cell.modelData)
                        onDoubleTapped: view.newEvent(cell.modelData)
                    }

                    Rectangle {
                        id: number
                        x: Tokens.s2
                        y: Tokens.s2
                        width: Math.max(24, dayText.implicitWidth + 12)
                        height: 24
                        radius: 12
                        color: cell.isToday ? Tokens.accent : "transparent"
                        Text {
                            id: dayText
                            anchors.centerIn: parent
                            text: cell.modelData.getDate() === 1
                                  ? cell.modelData.toLocaleDateString(Qt.locale(), i18nc("@info date format, QDate pattern", "MMM d"))
                                  : cell.modelData.getDate()
                            color: cell.isToday ? Tokens.textOnAccent : cell.inMonth ? Tokens.textPrimary : Tokens.textTertiary
                            font.family: "Inter"
                            font.pixelSize: 12
                            font.weight: cell.isToday ? Font.DemiBold : Font.Normal
                            font.features: { "tnum": 1 }
                        }
                    }

                    Column {
                        anchors.top: number.bottom
                        anchors.topMargin: Tokens.s1
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: Tokens.s2
                        spacing: 2
                        Repeater {
                            model: cell.dayOcc.length > cell.fits + 1 ? cell.dayOcc.slice(0, cell.fits) : cell.dayOcc
                            EventChip {
                                required property var modelData
                                width: parent.width
                                occ: modelData
                                onClicked: view.editOccurrence(modelData)
                            }
                        }
                        QQC2.AbstractButton {
                            visible: cell.dayOcc.length > cell.fits + 1
                            height: 18
                            width: parent.width
                            hoverEnabled: true
                            text: i18ncp("@action:button events of a day that do not fit", "+%1 more", "+%1 more",
                                         cell.dayOcc.length - cell.fits)
                            onClicked: view.dayOpened(cell.modelData)
                            contentItem: QQC2.Label {
                                leftPadding: Tokens.s2
                                text: parent.text
                                color: parent.hovered ? Tokens.accentText : Tokens.textSecondary
                                font: Tokens.caption
                            }
                            background: null
                        }
                    }
                }
            }
        }
    }
}

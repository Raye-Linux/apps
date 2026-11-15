// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The day and the week: the hours down the side, one column per day, the
// all-day events along the top, and a line at the current time. A click on
// an empty half hour starts a new event there.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui
import "datelogic.js" as D

Item {
    id: view

    property var model
    property date firstDay: new Date()
    property int days: 7
    property date now: new Date()
    readonly property real hourHeight: 52
    readonly property real gutter: 64
    readonly property var dayList: {
        const out = []
        for (let i = 0; i < days; ++i)
            out.push(D.addDays(D.dayStart(firstDay), i))
        return out
    }
    readonly property var occ: model.revision >= 0 ? model.occurrences(dayList[0], D.addDays(dayList[days - 1], 1)) : []
    readonly property var allDayOcc: occ.filter(o => o.allDay || (o.end - o.start) >= 24 * 3600000)
    readonly property real colWidth: (width - gutter) / days

    signal newEventAt(date start)
    signal editOccurrence(var occ)
    signal dayPicked(date day)

    function scrollToMorning() {
        const hour = D.sameDay(now, firstDay) || days > 1 ? Math.max(0, Math.min(now.getHours() - 1, 8)) : 8
        flick.contentY = hour * hourHeight
    }
    Component.onCompleted: scrollToMorning()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // the days, and the all-day strip under them
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 52 + allDayColumn.height
            Row {
                x: view.gutter
                Repeater {
                    model: view.dayList
                    QQC2.AbstractButton {
                        id: head
                        required property var modelData
                        readonly property bool isToday: D.sameDay(modelData, view.now)
                        width: view.colWidth
                        height: 52
                        hoverEnabled: true
                        onClicked: view.dayPicked(modelData)
                        background: Rectangle { color: head.hovered ? Tokens.fillSubtleHover : "transparent" }
                        contentItem: Column {
                            topPadding: Tokens.s2
                            leftPadding: Tokens.s3
                            QQC2.Label {
                                text: head.modelData.toLocaleDateString(Qt.locale(), "ddd")
                                color: head.isToday ? Tokens.accentText : Tokens.textSecondary
                                font: Tokens.caption
                            }
                            Rectangle {
                                width: Math.max(28, dayNum.implicitWidth + 12)
                                height: 28
                                radius: 14
                                x: -4
                                color: head.isToday ? Tokens.accent : "transparent"
                                Text {
                                    id: dayNum
                                    anchors.centerIn: parent
                                    text: head.modelData.getDate()
                                    color: head.isToday ? Tokens.textOnAccent : Tokens.textPrimary
                                    font.family: "Inter Display"
                                    font.pixelSize: 18
                                    font.weight: Font.DemiBold
                                    font.features: { "tnum": 1 }
                                }
                            }
                        }
                    }
                }
            }
            Column {
                id: allDayColumn
                y: 52
                x: view.gutter
                width: parent.width - view.gutter
                spacing: 2
                bottomPadding: view.allDayOcc.length ? Tokens.s2 : 0
                Repeater {
                    model: view.allDayOcc
                    Item {
                        id: strip
                        required property var modelData
                        readonly property real from: Math.max(0, Math.round((D.dayStart(modelData.start) - view.dayList[0]) / 86400000))
                        readonly property real to: Math.min(view.days, Math.round((modelData.end - view.dayList[0]) / 86400000))
                        width: parent.width
                        height: 22
                        EventChip {
                            x: strip.from * view.colWidth + 2
                            width: Math.max(1, strip.to - strip.from) * view.colWidth - 4
                            occ: strip.modelData
                            showTime: false
                            onClicked: view.editOccurrence(strip.modelData)
                        }
                    }
                }
            }
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: Tokens.strokeDivider
            }
        }

        Flickable {
            id: flick
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentHeight: 24 * view.hourHeight
            boundsBehavior: Flickable.StopAtBounds
            QQC2.ScrollBar.vertical: QQC2.ScrollBar {}

            // hours
            Repeater {
                model: 24
                Item {
                    required property int index
                    y: index * view.hourHeight
                    width: flick.width
                    height: view.hourHeight
                    QQC2.Label {
                        visible: parent.index > 0
                        width: view.gutter - Tokens.s3
                        y: -implicitHeight / 2
                        horizontalAlignment: Text.AlignRight
                        text: D.timeText(new Date(2000, 0, 1, parent.index))
                        color: Tokens.textTertiary
                        font: Tokens.caption
                    }
                    Rectangle {
                        x: view.gutter
                        width: parent.width - view.gutter
                        height: 1
                        color: Tokens.strokeDivider
                    }
                }
            }

            // the day columns
            Repeater {
                model: view.dayList
                Item {
                    id: column
                    required property var modelData
                    required property int index
                    readonly property var placed: D.lanes(view.occ.filter(o => !o.allDay && (o.end - o.start) < 24 * 3600000
                                                                                && o.start < D.addDays(modelData, 1) && o.end > modelData))
                    x: view.gutter + index * view.colWidth
                    width: view.colWidth
                    height: 24 * view.hourHeight

                    Rectangle {
                        width: 1
                        height: parent.height
                        color: Tokens.strokeDivider
                    }
                    TapHandler {
                        onTapped: point => {
                            const half = Math.floor(point.position.y / (view.hourHeight / 2))
                            const d = new Date(column.modelData)
                            d.setHours(Math.floor(half / 2), (half % 2) * 30)
                            view.newEventAt(d)
                        }
                    }

                    Repeater {
                        model: column.placed
                        QQC2.AbstractButton {
                            id: block
                            required property var modelData
                            readonly property var o: modelData.ev
                            readonly property color tint: o.color ? o.color : Tokens.accent
                            readonly property real yTop: D.minutesIn(o.start, column.modelData) / 60 * view.hourHeight
                            readonly property real yBottom: D.minutesIn(o.end, column.modelData) / 60 * view.hourHeight
                            x: 2 + modelData.lane * (column.width - 6) / modelData.count
                            y: yTop + 1
                            width: (column.width - 6) / modelData.count - 2
                            height: Math.max(20, yBottom - yTop - 2)
                            hoverEnabled: true
                            text: o.title !== "" ? o.title : i18nc("@info an event with no title", "(No title)")
                            Accessible.name: text
                            onClicked: view.editOccurrence(o)
                            background: Rectangle {
                                radius: Tokens.radiusControl
                                color: block.pressed ? Qt.alpha(block.tint, 0.4) : block.hovered ? Qt.alpha(block.tint, 0.34) : Qt.alpha(block.tint, 0.26)
                                border.width: 1
                                border.color: Qt.alpha(block.tint, 0.5)
                                Rectangle {
                                    width: 3
                                    height: parent.height - 4
                                    x: 2
                                    y: 2
                                    radius: 1.5
                                    color: block.tint
                                }
                            }
                            contentItem: Column {
                                leftPadding: 9
                                topPadding: 2
                                clip: true
                                QQC2.Label {
                                    width: block.width - 12
                                    text: block.text
                                    elide: Text.ElideRight
                                    color: Tokens.textPrimary
                                    font.family: Tokens.caption.family
                                    font.pointSize: Tokens.caption.pointSize
                                    font.weight: Font.DemiBold
                                }
                                QQC2.Label {
                                    visible: block.height > 36
                                    width: block.width - 12
                                    elide: Text.ElideRight
                                    text: D.timeText(block.o.start) + " – "
                                          + D.timeText(block.o.end)
                                          + (block.o.location ? " · " + block.o.location : "")
                                    color: Tokens.textSecondary
                                    font: Tokens.caption
                                }
                            }
                        }
                    }

                    // now
                    Rectangle {
                        visible: D.sameDay(column.modelData, view.now)
                        y: D.minutesIn(view.now, column.modelData) / 60 * view.hourHeight - 1
                        width: parent.width
                        height: 2
                        color: Tokens.statusCritical
                        Rectangle {
                            x: -4
                            y: -3
                            width: 8
                            height: 8
                            radius: 4
                            color: Tokens.statusCritical
                        }
                    }
                }
            }
        }
    }
}

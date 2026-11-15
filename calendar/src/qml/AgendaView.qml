// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// What is coming: the next three months, day by day, the empty days left out.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui
import "datelogic.js" as D

Item {
    id: view

    property var model
    property date fromDay: new Date()
    property date now: new Date()
    readonly property var occ: model.revision >= 0 ? model.occurrences(D.dayStart(fromDay), D.addDays(D.dayStart(fromDay), 92)) : []
    readonly property var rows: {
        const out = []
        let last = null
        for (const o of occ) {
            // an event that began before the first day shows on the first day
            const day = o.start < D.dayStart(fromDay) ? D.dayStart(fromDay) : D.dayStart(o.start)
            if (!last || !D.sameDay(last, day)) {
                out.push({ heading: true, day: day })
                last = day
            }
            out.push({ heading: false, occ: o, day: day })
        }
        return out
    }

    signal editOccurrence(var occ)

    QQC2.Label {
        anchors.centerIn: parent
        visible: view.rows.length === 0
        text: i18nc("@info", "Nothing in the next three months")
        color: Tokens.textSecondary
        font: Tokens.bodyLarge
    }

    ListView {
        id: list
        anchors.fill: parent
        anchors.leftMargin: Tokens.s7
        anchors.rightMargin: Tokens.s7
        clip: true
        model: view.rows
        QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
        delegate: Loader {
            required property var modelData
            width: ListView.view.width
            sourceComponent: modelData.heading ? heading : row

            Component {
                id: heading
                QQC2.Label {
                    topPadding: Tokens.s6
                    bottomPadding: Tokens.s2
                    text: {
                        const d = parent.modelData.day
                        const t = D.dayStart(view.now)
                        const word = D.sameDay(d, t) ? i18nc("@title day", "Today")
                                   : D.sameDay(d, D.addDays(t, 1)) ? i18nc("@title day", "Tomorrow") : ""
                        const date = d.toLocaleDateString(Qt.locale(), i18nc("@info date format, QDate pattern", "dddd, MMMM d"))
                        return word ? word + " · " + date : date
                    }
                    color: Tokens.textPrimary
                    font: Tokens.subtitle
                }
            }
            Component {
                id: row
                QQC2.AbstractButton {
                    id: item
                    readonly property var o: parent.modelData.occ
                    height: 52
                    hoverEnabled: !o.holiday
                    enabled: !o.holiday
                    onClicked: view.editOccurrence(o)
                    background: Rectangle {
                        radius: Tokens.radiusControl
                        color: item.hovered ? Tokens.fillSubtleHover : "transparent"
                    }
                    contentItem: RowLayout {
                        spacing: Tokens.s4
                        QQC2.Label {
                            Layout.preferredWidth: 120
                            Layout.leftMargin: Tokens.s3
                            text: item.o.allDay ? (item.o.holiday ? i18nc("@info", "Holiday") : i18nc("@info", "All day"))
                                  : D.timeText(item.o.start) + " – "
                                    + D.timeText(item.o.end)
                            color: Tokens.textSecondary
                            font.family: Tokens.body.family
                            font.pointSize: Tokens.body.pointSize
                            font.features: { "tnum": 1 }
                        }
                        Rectangle {
                            width: 4
                            height: 28
                            radius: 2
                            color: item.o.holiday ? Tokens.strokeStrong : item.o.color ? item.o.color : Tokens.accent
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0
                            QQC2.Label {
                                Layout.fillWidth: true
                                text: item.o.title !== "" ? item.o.title : i18nc("@info an event with no title", "(No title)")
                                elide: Text.ElideRight
                                color: Tokens.textPrimary
                                font: Tokens.bodyStrong
                            }
                            QQC2.Label {
                                Layout.fillWidth: true
                                visible: (item.o.location || "") !== ""
                                text: item.o.location || ""
                                elide: Text.ElideRight
                                color: Tokens.textSecondary
                                font: Tokens.caption
                            }
                        }
                        Row {
                            Layout.rightMargin: Tokens.s3
                            spacing: Tokens.s3
                            Glyph {
                                visible: item.o.repeats === true
                                kind: "repeat"
                                color: Tokens.textSecondary
                            }
                            Glyph {
                                visible: item.o.alarm === true
                                kind: "bell"
                                color: Tokens.textSecondary
                            }
                        }
                    }
                }
            }
        }
    }
}

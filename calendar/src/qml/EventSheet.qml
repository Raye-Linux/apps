// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Making and changing an event: what, when, how often, when to be
// reminded, its colour, where, and a note. Return saves; Esc leaves it be.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui
import "datelogic.js" as D

QQC2.Popup {
    id: sheet

    property var model
    property string uid: ""
    property date occurrenceStart
    property bool repeats: false

    property string title: ""
    property string location: ""
    property string notes: ""
    property bool allDay: false
    property date startDay: new Date()
    property int startMin: 9 * 60
    property date endDay: new Date()
    property int endMin: 10 * 60
    property string repeat: "none"
    // a rule from another calendar that this one cannot follow
    property bool repeatUnknown: false
    // -1: no reminder
    property int alarm: 15
    property string color: ""

    readonly property var timedAlarms: [
        { value: -1, text: i18nc("@item reminder", "None") },
        { value: 0, text: i18nc("@item reminder", "When it starts") },
        { value: 5, text: i18nc("@item reminder", "5 minutes before") },
        { value: 10, text: i18nc("@item reminder", "10 minutes before") },
        { value: 15, text: i18nc("@item reminder", "15 minutes before") },
        { value: 30, text: i18nc("@item reminder", "30 minutes before") },
        { value: 60, text: i18nc("@item reminder", "1 hour before") },
        { value: 120, text: i18nc("@item reminder", "2 hours before") },
        { value: 1440, text: i18nc("@item reminder", "1 day before") },
        { value: 10080, text: i18nc("@item reminder", "1 week before") },
    ]
    // an all-day reminder counts back from midnight: 9:00 that day is -540
    readonly property var dayAlarms: [
        { value: -1, text: i18nc("@item reminder", "None") },
        { value: -540, text: i18nc("@item reminder", "On the day, at 9:00 AM") },
        { value: 900, text: i18nc("@item reminder", "The day before, at 9:00 AM") },
        { value: 9540, text: i18nc("@item reminder", "A week before, at 9:00 AM") },
    ]
    readonly property var repeats_: [
        { value: "none", text: i18nc("@item repeat", "Does not repeat") },
        { value: "daily", text: i18nc("@item repeat", "Every day") },
        { value: "weekly", text: i18nc("@item repeat", "Every week on %1", startDay.toLocaleDateString(Qt.locale(), "dddd")) },
        { value: "monthly", text: i18nc("@item repeat", "Every month on day %1", startDay.getDate()) },
        { value: "yearly", text: i18nc("@item repeat", "Every year on %1", startDay.toLocaleDateString(Qt.locale(), i18nc("@info date format, QDate pattern", "MMMM d"))) },
    ]

    signal saved()

    function openNew(start, allDayEvent) {
        uid = ""
        repeats = false
        title = ""
        location = ""
        notes = ""
        allDay = allDayEvent
        startDay = D.dayStart(start)
        endDay = startDay
        startMin = allDayEvent ? 9 * 60 : start.getHours() * 60 + start.getMinutes()
        endMin = Math.min(24 * 60 - 1, startMin + 60)
        repeat = "none"
        repeatUnknown = false
        alarm = allDayEvent ? 900 : 15
        color = ""
        open()
        titleField.forceActiveFocus()
    }

    function openEdit(occ) {
        const e = model.event(occ.uid)
        if (!e.uid)
            return
        uid = e.uid
        occurrenceStart = occ.start
        title = e.title
        location = e.location
        notes = e.notes
        allDay = e.allDay
        startDay = D.dayStart(e.start)
        endDay = D.dayStart(e.end)
        startMin = e.start.getHours() * 60 + e.start.getMinutes()
        endMin = e.end.getHours() * 60 + e.end.getMinutes()
        repeat = e.repeat
        repeats = e.repeat !== "none"
        repeatUnknown = e.repeatUnknown
        alarm = e.hasAlarm ? e.alarmMinutes : -1
        color = e.color
        open()
        titleField.forceActiveFocus()
    }

    function at(day, minutes) {
        return new Date(day.getFullYear(), day.getMonth(), day.getDate(), Math.floor(minutes / 60), minutes % 60)
    }

    function save() {
        let start = at(startDay, allDay ? 0 : startMin)
        let end = at(endDay, allDay ? 0 : endMin)
        if (end < start)
            end = allDay ? start : new Date(start.getTime() + 3600000)
        const stored = sheet.model.save({
            uid: sheet.uid,
            title: titleField.text,
            location: locationField.text,
            notes: notesField.text,
            allDay: sheet.allDay,
            start: start,
            end: end,
            repeat: sheet.repeat,
            keepRule: sheet.repeatUnknown,
            hasAlarm: sheet.alarm !== -1,
            alarmMinutes: Math.max(-100000, sheet.alarm),
            color: sheet.color,
        })
        // the window says why when the file could not be written
        if (stored === "")
            return
        close()
        saved()
    }

    onAllDayChanged: {
        // keep a reminder that makes sense for the kind of event
        const list = allDay ? dayAlarms : timedAlarms
        if (!list.some(a => a.value === alarm))
            alarm = alarm === -1 ? -1 : (allDay ? 900 : 15)
    }

    parent: QQC2.Overlay.overlay
    anchors.centerIn: parent
    width: 520
    modal: true
    focus: true
    padding: Tokens.s7
    closePolicy: QQC2.Popup.CloseOnEscape

    QQC2.Overlay.modal: Rectangle { color: Tokens.bgSmoke }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Tokens.durationNormal; easing.type: Tokens.easingNormal }
            NumberAnimation { property: "scale"; from: 0.96; to: 1; duration: Tokens.durationNormal; easing.type: Tokens.easingNormal }
        }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; to: 0; duration: Tokens.durationExit; easing.type: Tokens.easingExit }
    }

    background: Rectangle {
        radius: Tokens.radiusOverlay
        color: Tokens.bgTertiary
        border.width: 1
        border.color: Tokens.strokeSurface
    }

    component FieldLabel: QQC2.Label {
        Layout.preferredWidth: 124
        color: Tokens.textSecondary
        font: Tokens.body
    }
    component Field: QQC2.TextField {
        id: f
        Layout.fillWidth: true
        implicitHeight: Tokens.sizeControlHeight
        color: Tokens.textPrimary
        placeholderTextColor: Tokens.textTertiary
        selectionColor: Tokens.accent
        selectedTextColor: Tokens.textOnAccent
        font: Tokens.body
        leftPadding: Tokens.s4
        background: Rectangle {
            radius: Tokens.radiusControl
            color: f.activeFocus ? Tokens.fillInputActive : f.hovered ? Tokens.fillControlHover : Tokens.fillControl
            border.width: 1
            border.color: Tokens.strokeControl
            Rectangle {
                anchors { left: parent.left; right: parent.right; bottom: parent.bottom; leftMargin: 1; rightMargin: 1 }
                height: f.activeFocus ? 2 : 1
                radius: Tokens.radiusControl
                color: f.activeFocus ? Tokens.accent : Tokens.strokeControlBottom
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: Tokens.s4
        Keys.onReturnPressed: sheet.save()
        Keys.onEnterPressed: sheet.save()

        QQC2.Label {
            text: sheet.uid === "" ? i18nc("@title", "New event") : i18nc("@title", "Edit event")
            color: Tokens.textPrimary
            font: Tokens.subtitle
        }

        Field {
            id: titleField
            text: sheet.title
            placeholderText: i18nc("@info:placeholder", "Add a title")
            font: Tokens.bodyLarge
            implicitHeight: 40
        }

        RowLayout {
            spacing: Tokens.s3
            FieldLabel { text: i18nc("@label", "All day") }
            Switch {
                text: i18nc("@label", "All day")
                showStateText: false
                checked: sheet.allDay
                onToggled: sheet.allDay = checked
            }
        }

        RowLayout {
            spacing: Tokens.s3
            FieldLabel { text: i18nc("@label", "Starts") }
            DateField {
                value: sheet.startDay
                onPicked: day => {
                    const shift = Math.round((D.dayStart(day) - sheet.startDay) / 86400000)
                    sheet.startDay = D.dayStart(day)
                    // the end follows, so the length stays the same
                    sheet.endDay = D.addDays(sheet.endDay, shift)
                }
            }
            TimeField {
                visible: !sheet.allDay
                minutes: sheet.startMin
                onPicked: m => {
                    const length = sheet.endMin - sheet.startMin
                    sheet.startMin = m
                    if (D.sameDay(sheet.startDay, sheet.endDay))
                        sheet.endMin = Math.min(24 * 60 - 1, m + Math.max(30, length))
                }
            }
        }

        RowLayout {
            spacing: Tokens.s3
            FieldLabel { text: i18nc("@label", "Ends") }
            DateField {
                value: sheet.endDay
                onPicked: day => sheet.endDay = day < sheet.startDay ? sheet.startDay : D.dayStart(day)
            }
            TimeField {
                visible: !sheet.allDay
                minutes: sheet.endMin
                onPicked: m => sheet.endMin = m
            }
        }

        RowLayout {
            spacing: Tokens.s3
            FieldLabel { text: i18nc("@label", "Repeat") }
            ChoiceBox {
                Layout.fillWidth: true
                options: sheet.repeats_
                value: sheet.repeat
                onPicked: v => { sheet.repeat = v; sheet.repeatUnknown = false }
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            Layout.leftMargin: 124 + Tokens.s3
            visible: sheet.repeatUnknown
            wrapMode: Text.Wrap
            text: i18nc("@info", "This event repeats in a way Calendar can't show yet, so only its first date appears. Pick another repeat to replace it.")
            color: Tokens.textSecondary
            font: Tokens.caption
        }

        RowLayout {
            spacing: Tokens.s3
            FieldLabel { text: i18nc("@label", "Reminder") }
            ChoiceBox {
                Layout.fillWidth: true
                options: sheet.allDay ? sheet.dayAlarms : sheet.timedAlarms
                value: sheet.alarm
                onPicked: v => sheet.alarm = v
            }
        }

        RowLayout {
            spacing: Tokens.s3
            FieldLabel { text: i18nc("@label", "Color") }
            Repeater {
                model: 6
                QQC2.AbstractButton {
                    id: swatch
                    required property int index
                    readonly property string value: sheet.model.colorFor(index)
                    readonly property bool on: value === sheet.color
                    implicitWidth: 28
                    implicitHeight: 28
                    hoverEnabled: true
                    Accessible.name: index === 0 ? i18nc("@info colour of an event", "Accent color")
                                                 : i18nc("@info colour of an event", "Color %1", index + 1)
                    Accessible.checked: on
                    onClicked: sheet.color = value
                    background: Rectangle {
                        radius: 14
                        color: "transparent"
                        border.width: swatch.on ? 2 : swatch.hovered ? 1 : 0
                        border.color: Tokens.textPrimary
                        Rectangle {
                            anchors.centerIn: parent
                            width: 20
                            height: 20
                            radius: 10
                            color: swatch.value ? swatch.value : Tokens.accent
                        }
                    }
                }
            }
        }

        RowLayout {
            spacing: Tokens.s3
            FieldLabel { text: i18nc("@label", "Location") }
            Field {
                id: locationField
                text: sheet.location
                placeholderText: i18nc("@info:placeholder", "Add a place")
            }
        }

        QQC2.TextArea {
            id: notesField
            Layout.fillWidth: true
            Layout.preferredHeight: 76
            text: sheet.notes
            placeholderText: i18nc("@info:placeholder", "Add a note")
            placeholderTextColor: Tokens.textTertiary
            wrapMode: TextEdit.Wrap
            color: Tokens.textPrimary
            selectionColor: Tokens.accent
            selectedTextColor: Tokens.textOnAccent
            font: Tokens.body
            leftPadding: Tokens.s4
            topPadding: Tokens.s3
            Keys.onReturnPressed: event => event.accepted = false
            background: Rectangle {
                radius: Tokens.radiusControl
                color: notesField.activeFocus ? Tokens.fillInputActive : Tokens.fillControl
                border.width: 1
                border.color: notesField.activeFocus ? Tokens.accent : Tokens.strokeControl
            }
        }

        RowLayout {
            Layout.topMargin: Tokens.s2
            spacing: Tokens.s2

            AccentButton {
                visible: sheet.uid !== ""
                accent: false
                text: i18nc("@action:button", "Delete")
                onClicked: {
                    if (!sheet.repeats) {
                        sheet.model.remove(sheet.uid)
                        sheet.close()
                    } else {
                        deleteMenu.open()
                    }
                }
                MenuPopup {
                    id: deleteMenu
                    y: -height - 4
                    items: [
                        { text: i18nc("@action:inmenu delete", "Only this one"),
                          action: () => { sheet.model.removeOne(sheet.uid, sheet.occurrenceStart); sheet.close() } },
                        { text: i18nc("@action:inmenu delete", "Every time it repeats"),
                          action: () => { sheet.model.remove(sheet.uid); sheet.close() } },
                    ]
                }
            }
            Item { Layout.fillWidth: true }
            AccentButton {
                accent: false
                text: i18nc("@action:button", "Cancel")
                onClicked: sheet.close()
            }
            AccentButton {
                text: i18nc("@action:button", "Save")
                onClicked: sheet.save()
            }
        }
    }
}

// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Adding or changing an alarm: the time on wheels, a name, the days it
// repeats, the sound (heard with the button beside it) and the snooze.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

Sheet {
    id: sheet

    property var clock
    property string key: ""
    property int hour: 7
    property int minute: 0
    property bool repeat: false
    property int days: 0x1f
    property string sound: "glow"
    property int snooze: 10
    readonly property bool twelve: clock ? clock.use12Hour : false

    readonly property var snoozeChoices: [
        { value: 0, text: i18nc("@item snooze", "Off") },
        { value: 5, text: i18ncp("@item snooze", "%1 minute", "%1 minutes", 5) },
        { value: 10, text: i18ncp("@item snooze", "%1 minute", "%1 minutes", 10) },
        { value: 20, text: i18ncp("@item snooze", "%1 minute", "%1 minutes", 20) },
        { value: 30, text: i18ncp("@item snooze", "%1 minute", "%1 minutes", 30) },
        { value: 60, text: i18nc("@item snooze", "1 hour") },
    ]

    function pad(n) { return n < 10 ? "0" + n : "" + n }

    function openNew() {
        key = ""
        hour = 7
        minute = 0
        repeat = false
        days = 0x1f
        sound = "glow"
        snooze = 10
        nameField.text = ""
        open()
    }
    function openEdit(a) {
        key = a.key
        hour = a.hour
        minute = a.minute
        repeat = a.days !== 0
        days = a.days !== 0 ? a.days : 0x1f
        sound = a.sound !== "" ? a.sound : "glow"
        snooze = a.snooze
        nameField.text = a.name
        open()
    }
    function save() {
        clock.saveAlarm(key, {
            name: nameField.text,
            hour: hour,
            minute: minute,
            days: repeat ? days : 0,
            sound: sound,
            snooze: snooze,
        })
        close()
    }

    heading: key === "" ? i18nc("@title:window", "Add new alarm") : i18nc("@title:window", "Edit alarm")
    onAccepted: save()
    onClosed: preview.stop()

    // The media libraries come with the sheet, not with the window: loading
    // them takes about 150 ms, done once the sheet is up so Play is instant.
    onOpened: makePreview.start()
    Timer {
        id: makePreview
        interval: 400
        onTriggered: preview.make()
    }
    QtObject {
        id: preview
        property var effect: null
        readonly property bool playing: effect !== null && effect.playing
        function make() {
            if (!effect)
                effect = Qt.createComponent("SoundPreview.qml").createObject(preview)
        }
        function play() {
            make()
            effect.source = sheet.clock ? sheet.clock.soundUrl(sheet.sound) : ""
            effect.play()
        }
        function stop() {
            if (effect)
                effect.stop()
        }
    }

    RowLayout {
        Layout.alignment: Qt.AlignHCenter
        spacing: Tokens.s2
        DigitWheel {
            labels: sheet.twelve ? Array.from({ length: 12 }, (_, i) => "" + (i === 0 ? 12 : i))
                                 : Array.from({ length: 24 }, (_, i) => sheet.pad(i))
            value: sheet.twelve ? sheet.hour % 12 : sheet.hour
            onMoved: v => sheet.hour = sheet.twelve ? v + (sheet.hour >= 12 ? 12 : 0) : v
            Accessible.name: i18nc("@label", "Hour")
        }
        Text {
            text: ":"
            color: Tokens.textSecondary
            font.family: "Inter Display"
            font.pixelSize: 28
        }
        DigitWheel {
            labels: Array.from({ length: 60 }, (_, i) => sheet.pad(i))
            value: sheet.minute
            onMoved: v => sheet.minute = v
            Accessible.name: i18nc("@label", "Minute")
        }
        DigitWheel {
            visible: sheet.twelve
            labels: [Qt.locale().amText, Qt.locale().pmText]
            value: sheet.hour >= 12 ? 1 : 0
            onMoved: v => sheet.hour = sheet.hour % 12 + (v ? 12 : 0)
            Accessible.name: i18nc("@label", "AM or PM")
        }
    }

    TextBox {
        id: nameField
        Layout.fillWidth: true
        placeholderText: i18nc("@info:placeholder", "Alarm name")
        maximumLength: 80
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Tokens.s3
        RowLayout {
            Layout.fillWidth: true
            QQC2.Label {
                Layout.fillWidth: true
                text: i18nc("@option:check", "Repeat alarm")
                color: Tokens.textPrimary
                font: Tokens.body
            }
            Switch {
                text: i18nc("@option:check", "Repeat alarm")
                showStateText: false
                checked: sheet.repeat
                onToggled: sheet.repeat = checked
            }
        }
        DayChips {
            Layout.alignment: Qt.AlignHCenter
            visible: sheet.repeat
            clock: sheet.clock
            days: sheet.days
            onToggled: d => sheet.days = d === 0 ? sheet.days : d
        }
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 2
        columnSpacing: Tokens.s4
        rowSpacing: Tokens.s3

        Glyph {
            kind: "sound"
            color: Tokens.textSecondary
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.s2
            ChoiceBox {
                Layout.fillWidth: true
                options: sheet.clock ? sheet.clock.sounds : []
                value: sheet.sound
                onPicked: v => { sheet.sound = v; preview.stop() }
                Accessible.name: i18nc("@label", "Alarm sound")
            }
            FlatButton {
                glyph: preview.playing ? "pause" : "play"
                tip: preview.playing ? i18nc("@action:button", "Stop") : i18nc("@action:button", "Play the sound")
                onClicked: preview.playing ? preview.stop() : preview.play()
            }
        }

        Glyph {
            kind: "snooze"
            color: Tokens.textSecondary
        }
        ChoiceBox {
            Layout.fillWidth: true
            options: sheet.snoozeChoices
            value: sheet.snooze
            onPicked: v => sheet.snooze = v
            Accessible.name: i18nc("@label", "Snooze time")
        }
    }

    buttons: [
        AccentButton {
            Layout.fillWidth: true
            text: i18nc("@action:button", "Save")
            enabled: !sheet.repeat || sheet.days !== 0
            onClicked: sheet.save()
        },
        AccentButton {
            Layout.fillWidth: true
            accent: false
            text: i18nc("@action:button", "Cancel")
            onClicked: sheet.close()
        },
        FlatButton {
            visible: sheet.key !== ""
            glyph: "delete"
            tip: i18nc("@action:button", "Delete alarm")
            onClicked: {
                sheet.clock.removeAlarm(sheet.key)
                sheet.close()
            }
        }
    ]
}

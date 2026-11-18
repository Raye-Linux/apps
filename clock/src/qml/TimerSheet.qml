// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Adding or changing a timer: hours, minutes and seconds on three wheels,
// and a name.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

Sheet {
    id: sheet

    property var clock
    property string key: ""
    property int hours: 0
    property int minutes: 15
    property int seconds: 0
    readonly property double lengthMs: ((hours * 60 + minutes) * 60 + seconds) * 1000

    function pad(n) { return n < 10 ? "0" + n : "" + n }
    function range(n) { return Array.from({ length: n }, (_, i) => pad(i)) }

    function openNew() {
        key = ""
        hours = 0
        minutes = 15
        seconds = 0
        nameField.text = ""
        open()
    }
    function openEdit(k, name, ms) {
        key = k
        const total = Math.round(ms / 1000)
        hours = Math.floor(total / 3600)
        minutes = Math.floor(total / 60) % 60
        seconds = total % 60
        nameField.text = name
        open()
    }
    function save() {
        if (lengthMs <= 0)
            return
        if (key === "")
            clock.addTimer(nameField.text, lengthMs)
        else
            clock.editTimer(key, nameField.text, lengthMs)
        close()
    }

    heading: key === "" ? i18nc("@title:window", "Add new timer") : i18nc("@title:window", "Edit timer")
    onAccepted: save()

    RowLayout {
        Layout.alignment: Qt.AlignHCenter
        spacing: Tokens.s2
        Repeater {
            model: [
                { labels: sheet.range(100), value: sheet.hours, unit: i18nc("@label short for hours", "hr") },
                { labels: sheet.range(60), value: sheet.minutes, unit: i18nc("@label short for minutes", "min") },
                { labels: sheet.range(60), value: sheet.seconds, unit: i18nc("@label short for seconds", "sec") },
            ]
            delegate: ColumnLayout {
                required property var modelData
                required property int index
                spacing: Tokens.s2
                DigitWheel {
                    labels: modelData.labels
                    value: modelData.value
                    onMoved: v => {
                        if (index === 0) sheet.hours = v
                        else if (index === 1) sheet.minutes = v
                        else sheet.seconds = v
                    }
                }
                QQC2.Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: modelData.unit
                    color: Tokens.textSecondary
                    font: Tokens.caption
                }
            }
        }
    }

    TextBox {
        id: nameField
        Layout.fillWidth: true
        placeholderText: i18nc("@info:placeholder", "Timer name")
        maximumLength: 80
    }

    buttons: [
        AccentButton {
            Layout.fillWidth: true
            text: i18nc("@action:button", "Save")
            enabled: sheet.lengthMs > 0
            onClicked: sheet.save()
        },
        AccentButton {
            Layout.fillWidth: true
            accent: false
            text: i18nc("@action:button", "Cancel")
            onClicked: sheet.close()
        }
    ]
}

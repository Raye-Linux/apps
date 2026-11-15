// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The calendar's window: the side pane with "New event", the small month
// and the holidays; the command bar with Today, the arrows, the date shown
// and the views; and one view at a time.
//
// Every colour, size and duration comes from Tokens.

import QtCore
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window
import org.raye.ui
import org.raye.calendar
import "datelogic.js" as D

MicaWindow {
    id: root

    width: 1180
    height: 760
    minimumWidth: 760
    minimumHeight: 520
    title: i18n("Calendar")
    visible: true

    readonly property var viewNames: [
        i18nc("@title:tab", "Day"),
        i18nc("@title:tab", "Week"),
        i18nc("@title:tab", "Month"),
        i18nc("@title:tab", "Agenda"),
    ]
    property date focusDay: new Date()
    property date now: new Date()
    readonly property int view: Math.max(0, Math.min(prefs.view, 3))
    readonly property date weekFirst: D.weekStart(focusDay, Qt.locale().firstDayOfWeek)

    // called by main.cpp for raye-calendar:?day= and ?new=
    function showDay(iso, isNew) {
        const d = D.fromIsoDay(iso) || new Date()
        focusDay = d
        if (isNew) {
            const start = new Date(d)
            const t = new Date()
            // today: the next whole hour; another day: 9:00
            start.setHours(D.sameDay(d, t) ? Math.min(23, t.getHours() + 1) : 9, 0)
            sheet.openNew(start, false)
        } else if (prefs.view === 2 || prefs.view === 3) {
            prefs.view = 0
        }
    }
    function importFile(url) {
        const n = calendar.importFile(url)
        if (n >= 0)
            notice.show(i18ncp("@info", "One event added", "%1 events added", n))
    }
    function move(delta) {
        if (view === 0)
            focusDay = D.addDays(focusDay, delta)
        else if (view === 1)
            focusDay = D.addDays(focusDay, 7 * delta)
        else if (view === 2)
            focusDay = new Date(focusDay.getFullYear(), focusDay.getMonth() + delta, 1)
        else
            focusDay = D.addDays(focusDay, 30 * delta)
    }
    readonly property string heading: {
        if (view === 0)
            return focusDay.toLocaleDateString(Qt.locale(), i18nc("@info date format, QDate pattern", "dddd, MMMM d, yyyy"))
        if (view === 1) {
            const last = D.addDays(weekFirst, 6)
            return weekFirst.getMonth() === last.getMonth()
                ? weekFirst.toLocaleDateString(Qt.locale(), "MMMM yyyy")
                : weekFirst.toLocaleDateString(Qt.locale(), "MMM") + " – " + last.toLocaleDateString(Qt.locale(), "MMM yyyy")
        }
        if (view === 2)
            return focusDay.toLocaleDateString(Qt.locale(), "MMMM yyyy")
        return i18nc("@title", "From %1", focusDay.toLocaleDateString(Qt.locale(), i18nc("@info date format, QDate pattern", "MMMM d")))
    }

    Settings {
        id: prefs
        category: "Window"
        property int view: 2
        property bool paneOpen: true
    }

    CalendarModel {
        id: calendar
    }

    // midnight and the line of "now" move on their own
    Timer {
        interval: 30000
        running: root.visible
        repeat: true
        onTriggered: root.now = new Date()
    }

    Shortcut { sequences: [StandardKey.New]; onActivated: sheet.openNew(root.newEventTime(), false) }
    Shortcut { sequence: "Ctrl+T"; onActivated: root.focusDay = new Date() }
    Shortcut { sequence: "Alt+Left"; onActivated: root.move(-1) }
    Shortcut { sequence: "Alt+Right"; onActivated: root.move(1) }
    Shortcut { sequence: "Ctrl+1"; onActivated: prefs.view = 0 }
    Shortcut { sequence: "Ctrl+2"; onActivated: prefs.view = 1 }
    Shortcut { sequence: "Ctrl+3"; onActivated: prefs.view = 2 }
    Shortcut { sequence: "Ctrl+4"; onActivated: prefs.view = 3 }

    function newEventTime() {
        const d = new Date(focusDay)
        const t = new Date()
        d.setHours(D.sameDay(d, t) ? Math.min(23, t.getHours() + 1) : 9, 0, 0, 0)
        return d
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // --- the side pane --------------------------------------------------
        Item {
            Layout.fillHeight: true
            Layout.preferredWidth: prefs.paneOpen ? 288 : 0
            visible: prefs.paneOpen
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Tokens.s5
                anchors.topMargin: Tokens.sizeTitlebar + Tokens.s4
                spacing: Tokens.s5

                AccentButton {
                    Layout.fillWidth: true
                    text: i18nc("@action:button", "New event")
                    onClicked: sheet.openNew(root.newEventTime(), false)
                }

                MiniMonth {
                    Layout.alignment: Qt.AlignHCenter
                    selected: root.focusDay
                    today: root.now
                    marked: {
                        const first = D.weekStart(new Date(shownYear, shownMonth, 1), Qt.locale().firstDayOfWeek)
                        const list = calendar.revision >= 0 ? calendar.occurrences(first, D.addDays(first, 42)) : []
                        const m = {}
                        for (const o of list) {
                            if (o.holiday)
                                continue
                            for (let d = D.dayStart(o.start); d < o.end || D.sameDay(d, o.start); d = D.addDays(d, 1))
                                m[D.isoDay(d)] = true
                        }
                        return m
                    }
                    onPicked: day => root.focusDay = day
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Tokens.strokeDivider
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Tokens.s2
                    QQC2.Label {
                        text: i18nc("@label", "Holidays")
                        color: Tokens.textPrimary
                        font: Tokens.bodyStrong
                    }
                    ChoiceBox {
                        Layout.fillWidth: true
                        options: calendar.holidayRegions.map(r => ({ value: r.id, text: r.name }))
                        value: calendar.holidayRegion
                        onPicked: v => calendar.holidayRegion = v
                    }
                }

                Item { Layout.fillHeight: true }

                QQC2.Label {
                    Layout.fillWidth: true
                    text: i18nc("@info", "Your events stay on this computer.")
                    wrapMode: Text.Wrap
                    color: Tokens.textTertiary
                    font: Tokens.caption
                }
            }
        }

        Rectangle {
            Layout.fillHeight: true
            width: 1
            visible: prefs.paneOpen
            color: Tokens.strokeDivider
        }

        // --- the bar and the view ------------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: Tokens.sizeTitlebar + Tokens.s2
                Layout.leftMargin: Tokens.s4
                Layout.rightMargin: Tokens.s5
                Layout.bottomMargin: Tokens.s3
                spacing: Tokens.s2

                FlatButton {
                    glyph: "menu"
                    tip: prefs.paneOpen ? i18nc("@info:tooltip", "Hide the side pane") : i18nc("@info:tooltip", "Show the side pane")
                    onClicked: prefs.paneOpen = !prefs.paneOpen
                }
                FlatButton {
                    glyph: "today"
                    text: i18nc("@action:button", "Today")
                    onClicked: root.focusDay = new Date()
                }
                FlatButton {
                    glyph: "back"
                    tip: i18nc("@info:tooltip", "Previous")
                    onClicked: root.move(-1)
                }
                FlatButton {
                    glyph: "forward"
                    tip: i18nc("@info:tooltip", "Next")
                    onClicked: root.move(1)
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: Tokens.s3
                    text: root.heading
                    elide: Text.ElideRight
                    color: Tokens.textPrimary
                    font: Tokens.subtitle
                }
                ViewSwitch {
                    names: root.viewNames
                    current: root.view
                    onChosen: index => prefs.view = index
                }
                FlatButton {
                    glyph: "more"
                    tip: i18nc("@info:tooltip", "More")
                    onClicked: moreMenu.open()
                    MenuPopup {
                        id: moreMenu
                        y: parent.height + 4
                        x: parent.width - width
                        items: [
                            { text: i18nc("@action:inmenu", "Import a calendar file…"), action: () => importDialog.open() },
                            { text: i18nc("@action:inmenu", "Export my calendar…"), action: () => exportDialog.open() },
                        ]
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Qt.alpha(Tokens.bgLayerSolid, root.mica ? 0.6 : 1)
                radius: Tokens.radiusWindow
                // only the top-left corner rounds into the pane, as in Windows' Mica layers
                Rectangle {
                    anchors.right: parent.right
                    width: parent.radius
                    height: parent.height
                    color: parent.color
                }
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: parent.radius
                    color: parent.color
                }

                Loader {
                    anchors.fill: parent
                    anchors.margins: Tokens.s2
                    sourceComponent: root.view === 2 ? monthView : root.view === 3 ? agendaView : timeView
                }
            }
        }
    }

    Component {
        id: monthView
        MonthView {
            model: calendar
            focusDay: root.focusDay
            today: root.now
            onDayPicked: day => root.focusDay = day
            onDayOpened: day => { root.focusDay = day; prefs.view = 0 }
            onNewEvent: day => {
                const d = new Date(day)
                d.setHours(9, 0)
                sheet.openNew(d, true)
            }
            onEditOccurrence: occ => sheet.openEdit(occ)
        }
    }
    Component {
        id: timeView
        TimeGrid {
            model: calendar
            days: root.view === 0 ? 1 : 7
            firstDay: root.view === 0 ? root.focusDay : root.weekFirst
            now: root.now
            onNewEventAt: start => sheet.openNew(start, false)
            onEditOccurrence: occ => sheet.openEdit(occ)
            onDayPicked: day => { root.focusDay = day; prefs.view = 0 }
        }
    }
    Component {
        id: agendaView
        AgendaView {
            model: calendar
            fromDay: root.focusDay
            now: root.now
            onEditOccurrence: occ => sheet.openEdit(occ)
        }
    }

    EventSheet {
        id: sheet
        model: calendar
        onSaved: notice.show(i18nc("@info", "Saved"))
    }

    FileDialog {
        id: importDialog
        title: i18nc("@title:window", "Import a calendar file")
        nameFilters: [i18nc("@item file type", "Calendar files (*.ics)")]
        onAccepted: root.importFile(selectedFile)
    }
    FileDialog {
        id: exportDialog
        title: i18nc("@title:window", "Export my calendar")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "ics"
        currentFile: StandardPaths.writableLocation(StandardPaths.DocumentsLocation) + "/"
                     + i18nc("@info file name, no extension", "My calendar") + ".ics"
        nameFilters: [i18nc("@item file type", "Calendar files (*.ics)")]
        onAccepted: if (calendar.exportFile(selectedFile)) notice.show(i18nc("@info", "Calendar exported"))
    }

    // a word at the bottom after a save, an import or an error
    Rectangle {
        id: notice
        property string message: calendar.lastError
        function show(text) {
            message = text
            hide.restart()
        }
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Tokens.s7
        width: noticeText.implicitWidth + 2 * Tokens.s6
        height: 36
        radius: 18
        color: Tokens.bgTooltip
        border.width: 1
        border.color: Tokens.strokeSurface
        opacity: hide.running || calendar.lastError !== "" ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: Tokens.durationFast } }
        Timer { id: hide; interval: 2400 }
        QQC2.Label {
            id: noticeText
            anchors.centerIn: parent
            text: calendar.lastError !== "" ? calendar.lastError : notice.message
            color: calendar.lastError !== "" ? Tokens.statusCritical : Tokens.textPrimary
            font: Tokens.body
        }
    }
}

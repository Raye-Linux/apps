// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The Control Panel's window. Three ways to see the same items, as in
// Windows: by category (home), large icons, small icons; and a page per
// category with each item's tasks. Search looks through the items, their
// tasks and System Tools.

import QtCore
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Window
import org.raye.ui
import org.raye.controlpanel
import "search.js" as Search

MicaWindow {
    id: root

    width: 1060
    height: 700
    minimumWidth: 640
    minimumHeight: 440
    title: i18n("Control Panel")
    visible: true

    // "" = home; a category id; or "admin" (System Tools)
    property string place: ""
    property var history: []

    Settings {
        id: prefs
        category: "ControlPanel"
        // category, large or small
        property string viewBy: "category"
    }

    Launcher { id: appLauncher }

    readonly property string filter: search.text.trim()
    readonly property var visibleItems: Catalogue.items.filter(i => canOpen(i))
    readonly property var sortedItems: visibleItems.slice().sort((a, b) => a.name.localeCompare(b.name))
    // one Terminal: Raye's own if it is there, the stock one otherwise
    readonly property var visibleTools: Catalogue.tools.filter(t =>
        appLauncher.hasProgram(t.program) && !(t.fallbackFor && appLauncher.hasProgram(t.fallbackFor)))
    // what a search looks through, worked out once: typing only filters it
    readonly property var searchable: sortedItems.map(i => ({ item: i, tasks: tasksOf(i) }))
    readonly property var results: Search.results(filter, searchable)
    readonly property var toolResults: filter === "" ? [] : visibleTools.filter(t => Search.hits(filter, t.name + " " + t.summary))

    // whether this PC can open it: a Settings page it shows here, a
    // program that is installed, or a page of the Control Panel
    function canOpen(target) {
        if (target.requires && !appLauncher.has(target.requires))
            return false
        if (target.page)
            return appLauncher.hasPage(target.page)
        if (target.programs)
            return appLauncher.firstProgram(target.programs) !== ""
        return !!target.panel
    }

    function tasksOf(item) {
        return (item.tasks || []).filter(t => canOpen(t))
    }

    function inCategory(item, id) {
        return item.category === id || (item.also || []).indexOf(id) >= 0
    }

    function categoryOf(id) {
        return Catalogue.categories.find(c => c.id === id)
    }

    // where we are, from Control Panel down
    readonly property var crumbs: {
        const list = [{ text: i18n("Control Panel"), place: "" }]
        if (place === "admin") {
            list.push({ text: categoryOf("security").name, place: "security" })
            list.push({ text: i18nc("@title", "System Tools"), place: "admin" })
        } else if (place !== "") {
            list.push({ text: categoryOf(place).name, place: place })
        } else if (prefs.viewBy !== "category") {
            list.push({ text: i18nc("@title", "All Control Panel Items"), place: "" })
        }
        return list
    }

    function go(where) {
        if (where === place && search.text === "")
            return
        history = history.concat([place])
        place = where
        search.text = ""
    }

    function back() {
        if (history.length === 0)
            return
        place = history[history.length - 1]
        history = history.slice(0, history.length - 1)
    }

    // the item just opened shows pressed until its program has a window
    property var launching: null

    function open(target) {
        if (!target.panel)
            launching = target
        if (target.panel)
            go(target.panel)
        else if (target.page)
            appLauncher.openPage(target.page)
        else if (target.programs)
            appLauncher.run(appLauncher.firstProgram(target.programs), target.args || [])
    }

    Shortcut { sequences: [StandardKey.Back, "Alt+Left", "Backspace"]; enabled: !search.activeFocus; onActivated: root.back() }
    Shortcut { sequences: [StandardKey.Find]; onActivated: search.forceActiveFocus() }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // back, up, where we are, and the search box
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Tokens.s3
            spacing: Tokens.s2

            Repeater {
                model: [
                    { glyph: "back", tip: i18nc("@action:button", "Back"), on: root.history.length > 0, act: () => root.back() },
                    { glyph: "up", tip: i18nc("@action:button", "Up one level"), on: root.crumbs.length > 1 && root.crumbs[root.crumbs.length - 2].place !== root.place,
                      act: () => root.go(root.crumbs[root.crumbs.length - 2].place) },
                ]
                delegate: QQC2.AbstractButton {
                    required property var modelData
                    implicitWidth: Tokens.sizeControlHeight
                    implicitHeight: Tokens.sizeControlHeight
                    enabled: modelData.on
                    hoverEnabled: true
                    opacity: enabled ? 1 : 0.4
                    Accessible.name: modelData.tip
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: modelData.tip
                    QQC2.ToolTip.delay: 600
                    onClicked: modelData.act()
                    background: Rectangle {
                        radius: Tokens.radiusControl
                        color: parent.pressed ? Tokens.fillSubtlePressed : parent.hovered ? Tokens.fillSubtleHover : "transparent"
                    }
                    contentItem: Item {
                        Glyph { anchors.centerIn: parent; kind: modelData.glyph }
                    }
                }
            }

            // the address: Control Panel › System and Security › ...
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Tokens.sizeControlHeight
                radius: Tokens.radiusControl
                color: Tokens.fillControl
                border.width: 1
                border.color: Tokens.strokeControl

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Tokens.s4
                    anchors.rightMargin: Tokens.s4
                    spacing: Tokens.s2
                    Repeater {
                        model: root.crumbs
                        delegate: RowLayout {
                            required property var modelData
                            required property int index
                            readonly property bool last: index === root.crumbs.length - 1
                            spacing: Tokens.s2
                            Glyph {
                                visible: index > 0
                                kind: "chevron"
                                color: Tokens.textSecondary
                                width: 12
                                height: 12
                            }
                            LinkText {
                                Layout.maximumWidth: 320
                                text: modelData.text
                                plain: true
                                enabled: !last
                                onClicked: root.go(modelData.place)
                            }
                        }
                    }
                    Item { Layout.fillWidth: true }
                }
            }

            TextBox {
                id: search
                Layout.preferredWidth: Math.min(300, root.width / 3)
                placeholderText: i18nc("@info:placeholder", "Search Control Panel")
                Keys.onEscapePressed: text = ""
            }
        }

        QQC2.ScrollView {
            id: scroller
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff

            ColumnLayout {
                width: scroller.availableWidth
                spacing: 0

                // ---- the home page by category
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.margins: Tokens.s8
                    visible: root.place === "" && prefs.viewBy === "category" && root.filter === ""
                    spacing: Tokens.s7

                    Header { text: i18nc("@title", "Adjust your computer's settings") }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: scroller.availableWidth > 900 ? 2 : 1
                        columnSpacing: Tokens.s10
                        rowSpacing: Tokens.s8
                        Repeater {
                            model: Catalogue.categories
                            delegate: CategoryBlock {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                Layout.alignment: Qt.AlignTop
                                links: modelData.links.filter(l => root.canOpen(l))
                                onCategoryOpened: id => root.go(id)
                                onLinkOpened: link => root.open(link)
                            }
                        }
                    }
                }

                // ---- a category: the others on the left, its items and
                // their tasks on the right
                RowLayout {
                    Layout.fillWidth: true
                    Layout.margins: Tokens.s8
                    visible: root.place !== "" && root.place !== "admin" && root.filter === ""
                    spacing: Tokens.s10

                    ColumnLayout {
                        Layout.alignment: Qt.AlignTop
                        Layout.preferredWidth: 220
                        Layout.maximumWidth: 260
                        visible: scroller.availableWidth > 820
                        spacing: Tokens.s1

                        LinkText {
                            Layout.bottomMargin: Tokens.s4
                            text: i18nc("@action:button", "Control Panel Home")
                            plain: true
                            onClicked: root.go("")
                        }
                        Repeater {
                            model: Catalogue.categories
                            delegate: QQC2.AbstractButton {
                                id: nav
                                required property var modelData
                                readonly property bool current: modelData.id === root.place
                                Layout.fillWidth: true
                                implicitHeight: Tokens.sizeControlHeight
                                hoverEnabled: true
                                focusPolicy: Qt.StrongFocus
                                Accessible.role: Accessible.Link
                                Accessible.name: modelData.name
                                onClicked: root.go(modelData.id)
                                background: Rectangle {
                                    radius: Tokens.radiusControl
                                    color: nav.current ? Tokens.fillSubtleSelected
                                         : nav.hovered ? Tokens.fillSubtleHover : "transparent"
                                    // the accent mark of the place we are in
                                    Rectangle {
                                        visible: nav.current
                                        anchors.left: parent.left
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 3
                                        height: 16
                                        radius: 1.5
                                        color: Tokens.accent
                                    }
                                    FocusRing { anchors.fill: parent; visible: nav.visualFocus }
                                }
                                contentItem: QQC2.Label {
                                    leftPadding: Tokens.s4
                                    verticalAlignment: Text.AlignVCenter
                                    text: nav.modelData.name
                                    elide: Text.ElideRight
                                    color: nav.current ? Tokens.textPrimary : Tokens.textSecondary
                                    font.family: Tokens.body.family
                                    font.pointSize: Tokens.body.pointSize
                                    font.weight: nav.current ? Font.DemiBold : Font.Normal
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: Tokens.s7

                        QQC2.Label {
                            text: root.place !== "" && root.place !== "admin" ? root.categoryOf(root.place).name : ""
                            color: Tokens.textPrimary
                            font: Tokens.title
                        }

                        Repeater {
                            model: root.sortedItems.filter(i => root.inCategory(i, root.place))
                            delegate: ItemBlock {
                                required property var modelData
                                Layout.fillWidth: true
                                name: modelData.name
                                summary: modelData.summary
                                iconName: modelData.icon
                                tasks: root.tasksOf(modelData)
                                showSummary: tasks.length === 0
                                onOpened: root.open(modelData)
                                onTaskOpened: task => root.open(task)
                            }
                        }
                    }
                }

                // ---- System Tools
                AdminToolsPage {
                    Layout.fillWidth: true
                    Layout.margins: Tokens.s8
                    visible: root.place === "admin" && root.filter === ""
                    tools: root.visibleTools
                    onToolOpened: tool => appLauncher.run(tool.program, tool.args || [])
                }

                // ---- every item: large or small icons
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.margins: Tokens.s8
                    visible: root.place === "" && prefs.viewBy !== "category" && root.filter === ""
                    spacing: Tokens.s5

                    Header { text: i18nc("@title", "All Control Panel Items") }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: Math.max(1, Math.floor(scroller.availableWidth / (prefs.viewBy === "small" ? 250 : 330)))
                        columnSpacing: Tokens.s3
                        rowSpacing: prefs.viewBy === "small" ? 0 : Tokens.s2
                        Repeater {
                            model: root.sortedItems
                            delegate: ItemTile {
                                required property var modelData
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                large: prefs.viewBy !== "small"
                                text: modelData.name
                                summary: modelData.summary
                                iconName: modelData.icon
                                starting: appLauncher.busy && root.launching === modelData
                                onClicked: root.open(modelData)
                            }
                        }
                    }
                }

                // ---- search results, wherever the search was typed
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.margins: Tokens.s8
                    visible: root.filter !== ""
                    spacing: Tokens.s7

                    QQC2.Label {
                        text: i18nc("@title", "Search results")
                        color: Tokens.textPrimary
                        font: Tokens.title
                    }

                    Repeater {
                        model: root.results
                        delegate: ItemBlock {
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.maximumWidth: 900
                            name: modelData.item.name
                            summary: modelData.item.summary
                            iconName: modelData.item.icon
                            tasks: modelData.tasks
                            showSummary: tasks.length === 0
                            onOpened: root.open(modelData.item)
                            onTaskOpened: task => root.open(task)
                        }
                    }

                    Repeater {
                        model: root.toolResults
                        delegate: ItemBlock {
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.maximumWidth: 900
                            name: modelData.name
                            summary: modelData.summary
                            glyph: modelData.glyph
                            showSummary: true
                            onOpened: appLauncher.run(modelData.program, modelData.args || [])
                        }
                    }

                    QQC2.Label {
                        visible: root.results.length === 0 && root.toolResults.length === 0
                        text: i18n("No items match your search.")
                        color: Tokens.textSecondary
                        font: Tokens.body
                    }
                }
            }
        }
    }

    // a page title with View by beside it, or under it in a narrow window
    component Header: GridLayout {
        property alias text: heading.text
        Layout.fillWidth: true
        columns: scroller.availableWidth > heading.implicitWidth + viewBy.implicitWidth + Tokens.s10 ? 2 : 1
        rowSpacing: Tokens.s4
        QQC2.Label {
            id: heading
            Layout.fillWidth: true
            elide: Text.ElideRight
            color: Tokens.textPrimary
            font: Tokens.title
        }
        ViewBy { id: viewBy }
    }

    // View by: Category / Large icons / Small icons
    component ViewBy: RowLayout {
        spacing: Tokens.s3
        QQC2.Label {
            text: i18nc("@label followed by Category, Large icons, Small icons", "View by:")
            color: Tokens.textSecondary
            font: Tokens.body
        }
        Rectangle {
            implicitWidth: choices.implicitWidth + 2 * Tokens.s1
            implicitHeight: Tokens.sizeControlHeight
            radius: Tokens.radiusControl
            color: Tokens.fillControl
            border.width: 1
            border.color: Tokens.strokeControl
            Row {
                id: choices
                anchors.centerIn: parent
                Repeater {
                    model: [
                        { id: "category", text: i18nc("@option:radio view by", "Category") },
                        { id: "large", text: i18nc("@option:radio view by", "Large icons") },
                        { id: "small", text: i18nc("@option:radio view by", "Small icons") },
                    ]
                    delegate: QQC2.AbstractButton {
                        required property var modelData
                        readonly property bool on: prefs.viewBy === modelData.id
                        implicitWidth: label.implicitWidth + 2 * Tokens.s4
                        implicitHeight: Tokens.sizeControlHeight - 2 * Tokens.s1
                        hoverEnabled: true
                        checkable: true
                        checked: on
                        Accessible.role: Accessible.RadioButton
                        Accessible.name: modelData.text
                        onClicked: prefs.viewBy = modelData.id
                        background: Rectangle {
                            radius: Tokens.radiusControl
                            color: parent.on ? Tokens.accent : parent.hovered ? Tokens.fillSubtleHover : "transparent"
                        }
                        contentItem: QQC2.Label {
                            id: label
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            text: modelData.text
                            color: parent.on ? Tokens.textOnAccent : Tokens.textPrimary
                            font: Tokens.body
                        }
                    }
                }
            }
        }
    }
}

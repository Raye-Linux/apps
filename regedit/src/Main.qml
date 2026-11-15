// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The Registry Editor's window: the menu bar, the address bar, the keys
// on the left and the values of the selected key on the right.

import QtCore
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window
import org.raye.ui
import org.raye.regedit

MicaWindow {
    id: root

    width: 1100
    height: 720
    minimumWidth: 720
    minimumHeight: 460
    title: i18n("Registry Editor")
    visible: true

    property string selectedValue: ""
    property string message: ""
    property bool messageIsError: false

    Settings {
        id: prefs
        category: "Window"
        property real treeWidth: 340
        property real nameWidth: 240
        property real typeWidth: 150
    }

    Registry {
        id: reg
        onRevealRequested: (index, valueName) => root.reveal(index, valueName)
        onFailed: text => root.say(text, true)
        onDone: text => root.say(text, false)
        onCurrentChanged: {
            address.text = reg.currentPath
            if (reg.values.indexOfName(root.selectedValue) < 0)
                root.selectedValue = ""
        }
    }

    function say(text, error) {
        message = text
        messageIsError = error
        messageTimer.restart()
    }

    Timer {
        id: messageTimer
        interval: 8000
        onTriggered: root.message = ""
    }

    function reveal(index, valueName) {
        tree.expandToIndex(index)
        tree.forceLayout()
        const r = tree.rowAtIndex(index)
        if (r >= 0)
            tree.positionViewAtRow(r, TableView.Contain)
        tree.selectionModel.setCurrentIndex(index, ItemSelectionModel.ClearAndSelect)
        root.selectedValue = valueName
        if (valueName !== "") {
            const i = reg.values.indexOfName(valueName)
            if (i >= 0)
                values.positionViewAtIndex(i, ListView.Contain)
        }
    }

    function chooseKey(index) {
        tree.selectionModel.setCurrentIndex(index, ItemSelectionModel.ClearAndSelect)
        reg.select(index)
        root.selectedValue = ""
    }

    function modify(name) {
        if (reg.values.indexOfName(name) < 0)
            return
        editSheet.valueName = name
        editSheet.locked = valueLocked(name) || !reg.currentWritable
        editData.text = valueRaw(name)
        editSheet.open()
        editData.forceActiveFocus()
        editData.selectAll()
    }

    function valueRaw(name) {
        return reg.values.rawOf(name)
    }
    function valueLocked(name) {
        return reg.values.lockedOf(name)
    }

    function isNumber(text) {
        return text !== "" && !isNaN(Number(text))
    }

    function newValue(kind) {
        newSheet.kind = kind
        newName.text = reg.uniqueName(i18nc("@item the name a new value starts with", "New value"))
        newData.text = kind === "YesNo" ? "false" : kind === "Number" ? "0" : kind === "List" && reg.currentDconf ? "[]" : ""
        newSheet.open()
        newName.forceActiveFocus()
        newName.selectAll()
    }

    readonly property var fileMenu: [
        { text: i18nc("@action:inmenu", "Import…"), action: () => importDialog.open() },
        { text: i18nc("@action:inmenu", "Export…"), shortcut: "", action: () => exportDialog.open() },
        { separator: true },
        { text: i18nc("@action:inmenu", "Exit"), action: () => Qt.quit() },
    ]
    readonly property var newItems: [
        { text: i18nc("@action:inmenu", "Text value"), enabled: reg.currentWritable, action: () => newValue("Text") },
        { text: i18nc("@action:inmenu", "Number value"), enabled: reg.currentWritable, action: () => newValue("Number") },
        { text: i18nc("@action:inmenu", "Yes/No value"), enabled: reg.currentWritable, action: () => newValue("YesNo") },
        { text: i18nc("@action:inmenu", "List value"), enabled: reg.currentWritable, action: () => newValue("List") },
    ]
    readonly property var editMenu: [
        { text: i18nc("@action:inmenu", "Modify…"), shortcut: i18nc("@item shortcut", "Enter"), enabled: root.selectedValue !== "", action: () => modify(root.selectedValue) },
        { separator: true },
    ].concat(newItems).concat([
        { separator: true },
        { text: i18nc("@action:inmenu", "Delete"), shortcut: i18nc("@item shortcut", "Del"), enabled: root.selectedValue !== "" && reg.currentWritable, action: () => deleteSheet.open() },
        { text: i18nc("@action:inmenu", "Undo"), shortcut: i18nc("@item shortcut", "Ctrl+Z"), enabled: reg.canUndo, action: () => reg.undo() },
        { separator: true },
        { text: i18nc("@action:inmenu", "Copy key name"), action: () => { address.selectAll(); address.copy(); address.deselect() } },
        { separator: true },
        { text: i18nc("@action:inmenu", "Find…"), shortcut: i18nc("@item shortcut", "Ctrl+F"), action: () => openFind() },
        { text: i18nc("@action:inmenu", "Find next"), shortcut: i18nc("@item shortcut", "F3"), enabled: findText.text !== "", action: () => findNext() },
    ])
    readonly property var viewMenu: [
        { text: i18nc("@action:inmenu", "Address bar"), action: () => addressRow.visible = !addressRow.visible },
        { text: i18nc("@action:inmenu", "Refresh"), shortcut: i18nc("@item shortcut", "F5"), action: () => reg.refresh() },
    ]
    function askImport(file) {
        const n = reg.prepareImport(file)
        if (n < 0)
            return
        importSheet.count = n
        importSheet.skipped = reg.importSkipped()
        importSheet.open()
    }

    readonly property var favouritesMenu: {
        const items = [
            { text: i18nc("@action:inmenu", "Add to Favorites…"), action: () => { favName.text = reg.currentPath.split("\\").pop(); favSheet.open(); favName.forceActiveFocus() } },
            { text: i18nc("@action:inmenu", "Remove Favorite…"), enabled: reg.favouriteNames.length > 0, action: () => removeFavSheet.open() },
        ]
        if (reg.favouriteNames.length > 0)
            items.push({ separator: true })
        for (const name of reg.favouriteNames)
            items.push({ text: name, action: () => reg.openFavourite(name) })
        return items
    }
    readonly property var helpMenu: [
        { text: i18nc("@action:inmenu", "About Registry Editor"), action: () => aboutSheet.open() },
    ]

    function openFind() {
        findSheet.open()
        findText.forceActiveFocus()
        findText.selectAll()
    }
    function findNext() {
        if (!reg.find(findText.text, findKeys.checked, findNames.checked, findData.checked, findWhole.checked))
            say(i18n("Finished searching through the registry."), false)
    }

    Shortcut { sequences: [StandardKey.Find]; onActivated: root.openFind() }
    Shortcut { sequence: "F3"; onActivated: findText.text !== "" ? root.findNext() : root.openFind() }
    Shortcut { sequences: [StandardKey.Refresh, "F5"]; onActivated: reg.refresh() }
    Shortcut { sequences: [StandardKey.Undo]; enabled: reg.canUndo; onActivated: reg.undo() }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // the menu bar
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Tokens.s3
            Layout.topMargin: Tokens.s2
            spacing: Tokens.s1
            MenuButton { text: i18nc("@title:menu", "File"); items: root.fileMenu }
            MenuButton { text: i18nc("@title:menu", "Edit"); items: root.editMenu }
            MenuButton { text: i18nc("@title:menu", "View"); items: root.viewMenu }
            MenuButton { text: i18nc("@title:menu", "Favorites"); items: root.favouritesMenu }
            MenuButton { text: i18nc("@title:menu", "Help"); items: root.helpMenu }
            Item { Layout.fillWidth: true }
        }

        // the address bar: shows the key, and goes to one typed or pasted in
        RowLayout {
            id: addressRow
            Layout.fillWidth: true
            Layout.margins: Tokens.s3
            spacing: Tokens.s3
            TextBox {
                id: address
                Layout.fillWidth: true
                text: reg.currentPath
                Accessible.name: i18nc("@label", "Address")
                onAccepted: {
                    const index = reg.indexOfPath(text)
                    if (index.valid) {
                        reg.select(index)
                        root.reveal(index, "")
                    } else {
                        root.say(i18n("There is no key named %1.", text), true)
                        text = reg.currentPath
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: Tokens.s3
            Layout.rightMargin: Tokens.s3
            spacing: 0

            // the keys
            Card {
                Layout.fillHeight: true
                Layout.preferredWidth: Math.max(220, Math.min(prefs.treeWidth, root.width - 380))

                TreeView {
                    id: tree
                    anchors.fill: parent
                    anchors.margins: Tokens.s2
                    clip: true
                    model: reg.tree
                    selectionModel: ItemSelectionModel {}
                    boundsBehavior: Flickable.StopAtBounds
                    columnWidthProvider: column => Math.max(tree.width, tree.implicitColumnWidth(column))
                    keyNavigationEnabled: true
                    pointerNavigationEnabled: false
                    QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
                    QQC2.ScrollBar.horizontal: QQC2.ScrollBar {}

                    delegate: TreeRow {
                        onChosen: root.chooseKey(tree.index(row, 0))
                        onMenuAsked: (x, y) => keyMenu.openAt(this, x, y)
                    }

                    Component.onCompleted: {
                        expand(0)
                        expand(1)
                    }

                    selectionBehavior: TableView.SelectRows
                    onCurrentRowChanged: {
                        const index = tree.index(currentRow, 0)
                        if (index.valid)
                            reg.select(index)
                    }
                }
            }

            // the splitter
            Item {
                Layout.fillHeight: true
                Layout.preferredWidth: Tokens.s3
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.SplitHCursor
                    property real startX
                    property real startWidth
                    onPressed: mouse => { startX = mapToItem(null, mouse.x, 0).x; startWidth = prefs.treeWidth }
                    onPositionChanged: mouse => prefs.treeWidth = Math.max(220, startWidth + mapToItem(null, mouse.x, 0).x - startX)
                }
            }

            // the values
            Card {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Tokens.s2
                    spacing: 0

                    Item {
                        Layout.fillWidth: true
                        implicitHeight: Tokens.sizeControlHeight
                        Repeater {
                            model: [
                                { x: 0, w: prefs.nameWidth, text: i18nc("@title:column", "Name") },
                                { x: prefs.nameWidth, w: prefs.typeWidth, text: i18nc("@title:column", "Type") },
                                { x: prefs.nameWidth + prefs.typeWidth, w: -1, text: i18nc("@title:column", "Data") },
                            ]
                            delegate: QQC2.Label {
                                required property var modelData
                                required property int index
                                x: modelData.x + (index === 0 ? Tokens.s4 + 16 + Tokens.s3 : 0)
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.text
                                color: Tokens.textSecondary
                                font: Tokens.caption
                            }
                        }
                        Repeater {
                            model: 2
                            delegate: Rectangle {
                                required property int index
                                x: (index === 0 ? prefs.nameWidth : prefs.nameWidth + prefs.typeWidth) - Tokens.s3
                                width: 1
                                height: parent.height - Tokens.s4
                                anchors.verticalCenter: parent.verticalCenter
                                color: Tokens.strokeDivider
                                MouseArea {
                                    anchors.fill: parent
                                    anchors.margins: -4
                                    cursorShape: Qt.SplitHCursor
                                    onPositionChanged: mouse => {
                                        const dx = mapToItem(parent.parent, mouse.x, 0).x - parent.x
                                        if (parent.index === 0)
                                            prefs.nameWidth = Math.max(120, prefs.nameWidth + dx)
                                        else
                                            prefs.typeWidth = Math.max(80, prefs.typeWidth + dx)
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

                    ListView {
                        id: values
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: reg.values
                        boundsBehavior: Flickable.StopAtBounds
                        QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
                        delegate: ValueRow {
                            width: values.width
                            nameWidth: prefs.nameWidth
                            typeWidth: prefs.typeWidth
                            selected: root.selectedValue === name
                            onChosen: root.selectedValue = name
                            onOpened: root.modify(name)
                            onMenuAsked: (x, y) => valueMenu.openAt(this, x, y)
                        }

                        // a right click on the empty part: New
                        TapHandler {
                            acceptedButtons: Qt.RightButton
                            onTapped: eventPoint => {
                                if (values.itemAt(eventPoint.position.x, eventPoint.position.y + values.contentY) === null)
                                    emptyMenu.openAt(values, eventPoint.position.x, eventPoint.position.y)
                            }
                        }
                        Keys.onDeletePressed: if (root.selectedValue !== "" && reg.currentWritable) deleteSheet.open()
                        Keys.onReturnPressed: if (root.selectedValue !== "") root.modify(root.selectedValue)

                        QQC2.Label {
                            anchors.centerIn: parent
                            width: parent.width - 2 * Tokens.s8
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            visible: reg.values.count === 0
                            text: reg.currentWritable ? i18n("This key has no values. Right-click to add one.")
                                                      : i18n("This key has no values of its own. Open it to see what is inside.")
                            color: Tokens.textSecondary
                            font: Tokens.body
                        }
                    }
                }
            }
        }

        // the status line
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Tokens.s3
            Layout.leftMargin: Tokens.s4
            spacing: Tokens.s3
            Glyph {
                visible: root.message === "" && ((reg.currentSystem && reg.currentWritable) || (!reg.currentWritable && reg.values.count > 0))
                kind: reg.currentSystem && reg.currentWritable ? "shield" : "lock"
                color: Tokens.textSecondary
            }
            QQC2.Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
                text: root.message !== "" ? root.message
                    : reg.busy ? i18n("Waiting for the password…")
                    : reg.currentNote !== "" ? reg.currentNote
                    : reg.currentSystem && reg.currentWritable ? i18n("Changes here apply to everyone on this PC and need root access.")
                    : !reg.currentWritable && reg.values.count > 0 ? i18n("These values are read only.")
                    : i18n("A copy of each file is kept before it is changed. Ctrl+Z takes a change back.")
                color: root.message !== "" && root.messageIsError ? Tokens.statusCritical : Tokens.textSecondary
                font: Tokens.caption
            }
        }
    }

    ContextMenu {
        id: valueMenu
        items: [
            { text: i18nc("@action:inmenu", "Modify…"), action: () => root.modify(root.selectedValue) },
            { separator: true },
            { text: i18nc("@action:inmenu", "Delete"), enabled: reg.currentWritable && !root.valueLocked(root.selectedValue), action: () => deleteSheet.open() },
            { text: i18nc("@action:inmenu", "Copy data"), action: () => { clip.text = root.valueRaw(root.selectedValue); clip.selectAll(); clip.copy() } },
        ]
    }
    ContextMenu {
        id: emptyMenu
        items: root.newItems
    }
    ContextMenu {
        id: keyMenu
        items: root.newItems.concat([
            { separator: true },
            { text: i18nc("@action:inmenu", "Export…"), action: () => exportDialog.open() },
            { text: i18nc("@action:inmenu", "Add to Favorites…"), action: () => { favName.text = reg.currentPath.split("\\").pop(); favSheet.open() } },
            { text: i18nc("@action:inmenu", "Copy key name"), action: () => { address.selectAll(); address.copy(); address.deselect() } },
        ])
    }
    // the clipboard is reached through a text field
    TextEdit { id: clip; visible: false }

    Sheet {
        id: editSheet
        property string valueName
        property bool locked: false
        readonly property string problem: locked ? "" : reg.checkValue(valueName, editData.text)
        heading: locked ? i18nc("@title:window", "Value") : i18nc("@title:window", "Edit value")
        okEnabled: !locked && problem === ""
        showCancel: !locked
        okText: locked ? i18nc("@action:button", "Close") : i18nc("@action:button", "OK")
        onAccepted: if (!locked) reg.setValue(valueName, editData.text)

        QQC2.Label { text: i18nc("@label", "Value name"); color: Tokens.textSecondary; font: Tokens.caption }
        TextBox { Layout.fillWidth: true; text: editSheet.valueName; readOnly: true }
        QQC2.Label { text: i18nc("@label", "Value data"); color: Tokens.textSecondary; font: Tokens.caption }
        TextBox {
            id: editData
            Layout.fillWidth: true
            readOnly: editSheet.locked
            onAccepted: editSheet.accept()
        }
        QQC2.Label {
            Layout.fillWidth: true
            visible: editSheet.problem !== "" || editSheet.locked
            wrapMode: Text.Wrap
            text: editSheet.locked ? i18n("This value is locked for everyone on this PC, or the file cannot be changed here.") : editSheet.problem
            color: editSheet.locked ? Tokens.textSecondary : Tokens.statusCritical
            font: Tokens.caption
        }
    }

    Sheet {
        id: newSheet
        property string kind: "Text"
        heading: kind === "Number" ? i18nc("@title:window", "New number value")
               : kind === "YesNo" ? i18nc("@title:window", "New Yes/No value")
               : kind === "List" ? i18nc("@title:window", "New list value")
               : i18nc("@title:window", "New text value")
        okEnabled: newName.text.trim() !== "" && newName.text.indexOf("=") < 0
                   && (kind !== "Number" || root.isNumber(newData.text.trim()))
                   && (kind !== "YesNo" || newData.text === "true" || newData.text === "false")
        onAccepted: reg.newValue(kind, newName.text.trim(), newData.text)

        QQC2.Label { text: i18nc("@label", "Value name"); color: Tokens.textSecondary; font: Tokens.caption }
        TextBox { id: newName; Layout.fillWidth: true; onAccepted: newData.forceActiveFocus() }
        QQC2.Label { text: i18nc("@label", "Value data"); color: Tokens.textSecondary; font: Tokens.caption }
        TextBox { id: newData; Layout.fillWidth: true; onAccepted: newSheet.accept() }
        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            visible: newSheet.kind === "YesNo" || (newSheet.kind === "List" && reg.currentDconf)
            text: newSheet.kind === "YesNo" ? i18n("Type true or false.") : i18n("Write the list as ['one', 'two'].")
            color: Tokens.textSecondary
            font: Tokens.caption
        }
    }

    Sheet {
        id: deleteSheet
        heading: i18nc("@title:window", "Delete this value?")
        okText: i18nc("@action:button", "Delete")
        onAccepted: {
            reg.deleteValue(root.selectedValue)
            root.selectedValue = ""
        }
        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: i18n("%1 goes back to its default value in the apps that read it. Ctrl+Z puts it back.", root.selectedValue)
            color: Tokens.textSecondary
            font: Tokens.body
        }
    }

    Sheet {
        id: findSheet
        heading: i18nc("@title:window", "Find")
        okText: i18nc("@action:button", "Find next")
        okEnabled: findText.text !== "" && (findKeys.checked || findNames.checked || findData.checked)
        onAccepted: root.findNext()

        QQC2.Label { text: i18nc("@label", "Find what"); color: Tokens.textSecondary; font: Tokens.caption }
        TextBox { id: findText; Layout.fillWidth: true; onAccepted: findSheet.accept() }
        QQC2.Label { text: i18nc("@label", "Look at"); color: Tokens.textSecondary; font: Tokens.caption }
        CheckRow { id: findKeys; text: i18nc("@option:check", "Keys"); checked: true }
        CheckRow { id: findNames; text: i18nc("@option:check", "Values"); checked: true }
        CheckRow { id: findData; text: i18nc("@option:check", "Data"); checked: true }
        CheckRow { id: findWhole; text: i18nc("@option:check", "Match whole string only") }
    }

    Sheet {
        id: favSheet
        heading: i18nc("@title:window", "Add to Favorites")
        okEnabled: favName.text.trim() !== ""
        onAccepted: reg.addFavourite(favName.text.trim())
        QQC2.Label { text: i18nc("@label", "Favorite name"); color: Tokens.textSecondary; font: Tokens.caption }
        TextBox { id: favName; Layout.fillWidth: true; onAccepted: favSheet.accept() }
    }

    Sheet {
        id: removeFavSheet
        heading: i18nc("@title:window", "Remove Favorite")
        okText: i18nc("@action:button", "Remove")
        property string chosen: ""
        okEnabled: chosen !== ""
        onAccepted: reg.removeFavourite(chosen)
        Repeater {
            model: reg.favouriteNames
            delegate: CheckRow {
                required property string modelData
                text: modelData
                checked: removeFavSheet.chosen === modelData
                onClicked: removeFavSheet.chosen = modelData
            }
        }
    }

    Sheet {
        id: importSheet
        property int count: 0
        property int skipped: 0
        heading: i18nc("@title:window", "Import into the registry?")
        okText: i18nc("@action:button", "Import")
        okEnabled: count > 0
        onAccepted: reg.applyImport()
        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: importSheet.count === 0 ? i18n("Nothing in this file can be imported here.")
                : i18np("One value will be changed. A copy of each file is kept first.",
                        "%1 values will be changed. A copy of each file is kept first.", importSheet.count)
            color: Tokens.textPrimary
            font: Tokens.body
        }
        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            visible: importSheet.skipped > 0
            text: i18np("One value is skipped: its key is not here, or cannot be changed.",
                        "%1 values are skipped: their key is not here, or cannot be changed.", importSheet.skipped)
            color: Tokens.textSecondary
            font: Tokens.caption
        }
    }

    Sheet {
        id: aboutSheet
        heading: i18nc("@title:window", "Registry Editor")
        showCancel: false
        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: i18n("Version %1", Qt.application.version)
            color: Tokens.textSecondary
            font: Tokens.body
        }
        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: i18n("Shows the settings of your apps and of the system as one tree, and changes them the careful way: only the value's line changes, a copy of the file is kept first, and Ctrl+Z takes a change back.")
            color: Tokens.textPrimary
            font: Tokens.body
        }
    }

    FileDialog {
        id: exportDialog
        title: i18nc("@title:window", "Export registry file")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "rayereg"
        nameFilters: [i18nc("@item file type", "Registry files (*.rayereg)")]
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: reg.exportTo(selectedFile, true)
    }

    FileDialog {
        id: importDialog
        title: i18nc("@title:window", "Import registry file")
        fileMode: FileDialog.OpenFile
        nameFilters: [i18nc("@item file type", "Registry files (*.rayereg)")]
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: root.askImport(selectedFile)
    }
}

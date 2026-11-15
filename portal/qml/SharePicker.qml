// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

// The window that asks what to share when an app wants the screen:
// "Discord wants to share your screen", tabs Entire screen / Window /
// Area, a grid of live pictures, then Share or Cancel. Shown by the
// backend in place of the desktop's chooser; portal/README.md lists what
// it is given and what the backend reads back.
import QtQuick
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels
import org.raye.ui

import "PickerLogic.js" as Logic

MicaWindow {
    id: root

    // --- what the backend gives (portal/README.md) ---------------------
    property string mainText
    property string appId
    property string appName
    property string appIconName
    property bool appKnown: false
    property var outputsModel: null
    property var windowsModel: null
    property bool multiple: false
    property int sourceTypes: Logic.SourceMonitor
    property int cursorMode: Logic.CursorHidden
    property bool persistRequested: false

    // --- what it reads back --------------------------------------------
    readonly property bool allowRestore: persistRequested && rememberOption.checked
    property alias showPointer: pointerOption.checked

    signal accept()
    signal reject()
    signal clearSelection()

    // the tests and the harness draw their own pictures; null is live
    property Component previewComponent: null

    readonly property bool canShare: (outputsModel?.hasSelection ?? false)
                                     || (windowsModel?.hasSelection ?? false)
    property bool keyboardUsed: false
    property string currentTab: tabList.length > 0 ? tabList[0] : ""
    property var tabList: []
    property bool finished: false

    // 32 px margins and 24 px gaps: three 216 px cards fill the 760 px
    // window exactly
    readonly property int margin: Tokens.s8
    readonly property int gap: Tokens.s7

    function present() {
        finished = false
        show()
        raise()
        requestActivate()
        intro.restart()
        Qt.callLater(() => (grid.count > 0 ? grid : tabs).forceActiveFocus(Qt.OtherFocusReason))
    }

    function share() {
        if (!canShare || finished)
            return
        finished = true
        accept()
    }

    function cancel() {
        if (finished)
            return
        finished = true
        reject()
    }

    // the backend shows the window again when the user gives up picking
    // an area: it starts over from present()
    onAccept: hide()
    onReject: hide()
    onClosing: cancel()

    // where Tab goes after the cards
    function firstFooterControl() {
        if (pointerOption.parent.visible)
            return pointerOption
        if (rememberOption.visible)
            return rememberOption
        return shareButton.enabled ? shareButton : cancelButton
    }

    // --- the screens model, row by row ---------------------------------
    function outputRows() {
        return outputsModel ? outputsModel.rowCount() : 0
    }
    function syntheticRows() {
        return outputsModel ? Number(outputsModel.syntheticCount ?? 0) : 0
    }
    function kindOfOutput(row) {
        return Logic.outputKind(row, outputRows(), syntheticRows(), sourceTypes)
    }
    function updateTabs() {
        const list = Logic.tabs(sourceTypes, outputsModel !== null, windowsModel !== null,
                                outputRows(), syntheticRows())
        tabList = list
        screensModel.invalidateFilter()
        areaModel.invalidateFilter()
        if (list.indexOf(currentTab) < 0)
            currentTab = list.length > 0 ? list[0] : ""
    }

    function tabText(id) {
        switch (id) {
        case "screen": return i18nc("@title:tab share a whole screen", "Entire screen")
        case "window": return i18nc("@title:tab share one window", "Window")
        case "area": return i18nc("@title:tab share a part of the screen", "Area")
        }
        return id
    }

    readonly property var shownModel: currentTab === "window" ? windowsModel
                                    : currentTab === "area" ? areaModel
                                    : currentTab === "screen" ? screensModel : null

    function sourceRow(row) {
        if (shownModel === windowsModel)
            return row
        return shownModel.mapToSource(shownModel.index(row, 0)).row
    }

    function isChosen(row) {
        return shownModel && shownModel.data(shownModel.index(row, 0), Qt.CheckStateRole) === Qt.Checked
    }

    function choose(row) {
        if (!shownModel || row < 0 || row >= shownModel.rowCount())
            return
        const index = shownModel.index(row, 0)
        if (multiple) {
            shownModel.setData(index, isChosen(row) ? Qt.Unchecked : Qt.Checked, Qt.CheckStateRole)
        } else {
            clearSelection()
            shownModel.setData(index, Qt.Checked, Qt.CheckStateRole)
        }
    }

    // a double click: just this one, shared right away
    function chooseAndShare(row) {
        if (!shownModel || row < 0 || row >= shownModel.rowCount())
            return
        clearSelection()
        shownModel.setData(shownModel.index(row, 0), Qt.Checked, Qt.CheckStateRole)
        share()
    }

    // Enter on a card: that card, or with several allowed what is chosen
    function enterOn(row) {
        if (!multiple || !canShare)
            choose(row)
        share()
    }

    KItemModels.KSortFilterProxyModel {
        id: screensModel
        sourceModel: root.outputsModel
        filterRowCallback: (row, parent) => Logic.tabOfKind(root.kindOfOutput(row)) === "screen"
    }
    KItemModels.KSortFilterProxyModel {
        id: areaModel
        sourceModel: root.outputsModel
        filterRowCallback: (row, parent) => Logic.tabOfKind(root.kindOfOutput(row)) === "area"
    }
    Connections {
        target: root.outputsModel
        ignoreUnknownSignals: true
        function onRowsInserted() { root.updateTabs() }
        function onRowsRemoved() { root.updateTabs() }
        function onModelReset() { root.updateTabs() }
    }

    onCurrentTabChanged: {
        grid.currentIndex = 0
        grid.positionViewAtBeginning()
    }

    Component.onCompleted: {
        updateTabs()
        const row = Logic.preselectedRow(sourceTypes, outputRows(), syntheticRows())
        if (row >= 0 && outputsModel)
            outputsModel.setData(outputsModel.index(row, 0), Qt.Checked, Qt.CheckStateRole)
    }

    // --- the window ------------------------------------------------------
    title: i18nc("@title:window", "Share your screen")
    width: 760
    height: 540
    minimumWidth: 760
    minimumHeight: 540
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint
    visible: false

    Item {
        id: stage
        anchors.fill: parent
        transformOrigin: Item.Center
        focus: true

        Accessible.role: Accessible.Dialog
        Accessible.name: headline.plainText
        Accessible.description: root.mainText

        // opens with the 167 ms scale-in of Raye dialogs
        ParallelAnimation {
            id: intro
            NumberAnimation { target: stage; property: "scale"; from: 0.96; to: 1; duration: Tokens.durationFast; easing.type: Tokens.easingFast }
            NumberAnimation { target: stage; property: "opacity"; from: 0; to: 1; duration: Tokens.durationFast; easing.type: Tokens.easingFast }
        }

        Keys.onPressed: event => {
            root.keyboardUsed = true
            event.accepted = false
        }
        Keys.onEscapePressed: event => { root.cancel(); event.accepted = true }
        Keys.onReturnPressed: event => { root.share(); event.accepted = true }
        Keys.onEnterPressed: event => { root.share(); event.accepted = true }

        // --- who asks ------------------------------------------------------
        Row {
            id: header
            x: root.margin
            y: Tokens.s7
            width: parent.width - 2 * root.margin
            spacing: Tokens.s4

            Kirigami.Icon {
                id: appIcon
                anchors.verticalCenter: parent.verticalCenter
                width: Tokens.sizeIconMultiline
                height: width
                source: root.appIconName !== "" ? root.appIconName : "application-x-executable"
                Accessible.ignored: true
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                width: header.width - appIcon.width - header.spacing
                spacing: Tokens.s1

                Text {
                    id: headline
                    objectName: "headline"
                    width: parent.width
                    readonly property string who: root.appKnown
                        ? "<b>" + Logic.escapeHtml(root.appName) + "</b>"
                        : Logic.escapeHtml(root.appName)
                    readonly property string plainText: {
                        const kind = Logic.requestKind(root.sourceTypes)
                        const name = root.appName
                        if (kind === "window")
                            return i18nc("@title %1 is the name of an app", "%1 wants to share a window", name)
                        if (kind === "virtual")
                            return i18nc("@title %1 is the name of an app", "%1 wants to share a new virtual screen", name)
                        return i18nc("@title %1 is the name of an app", "%1 wants to share your screen", name)
                    }
                    text: {
                        const kind = Logic.requestKind(root.sourceTypes)
                        if (kind === "window")
                            return i18nc("@title %1 is the name of an app", "%1 wants to share a window", who)
                        if (kind === "virtual")
                            return i18nc("@title %1 is the name of an app", "%1 wants to share a new virtual screen", who)
                        return i18nc("@title %1 is the name of an app", "%1 wants to share your screen", who)
                    }
                    textFormat: Text.StyledText
                    font: Tokens.bodyLarge
                    color: Tokens.textPrimary
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                Text {
                    textFormat: Text.PlainText
                    width: parent.width
                    text: root.multiple
                          ? i18nc("@info", "You can choose more than one.")
                          : i18nc("@info", "Choose what you want to share.")
                    font: Tokens.body
                    color: Tokens.textSecondary
                    elide: Text.ElideRight
                }
            }
        }

        // --- what kind ----------------------------------------------------
        SegmentedTabs {
            id: tabs
            objectName: "tabs"
            x: root.margin
            anchors.top: header.bottom
            anchors.topMargin: Tokens.s6
            visible: root.tabList.length > 1
            height: visible ? implicitHeight : 0
            tabs: root.tabList.map(id => ({ id: id, text: root.tabText(id) }))
            current: root.currentTab
            keyboardUsed: root.keyboardUsed
            onActivated: id => root.currentTab = id
            KeyNavigation.down: grid
        }

        // --- the cards -----------------------------------------------------
        Item {
            id: gridArea
            anchors.top: tabs.bottom
            anchors.topMargin: Tokens.s5
            anchors.bottom: footer.top
            anchors.left: parent.left
            anchors.right: parent.right

            GridView {
                id: grid
                objectName: "grid"

                // the cards line up with the text above: the margin less
                // half a gap, since each card sits in the middle of its cell
                readonly property int columns: Logic.columns(gridArea.width - 2 * root.margin + root.gap, cellWidth)

                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                width: columns * cellWidth
                cellWidth: 216 + root.gap
                cellHeight: 140 + Tokens.s3 + Tokens.bodyLineHeight + Tokens.s5
                topMargin: Tokens.s1
                bottomMargin: Tokens.s3
                clip: true
                model: root.shownModel
                activeFocusOnTab: count > 0
                keyNavigationEnabled: true
                keyNavigationWraps: false
                boundsBehavior: Flickable.StopAtBounds
                highlightFollowsCurrentItem: false
                currentIndex: 0

                Accessible.role: Accessible.List
                Accessible.name: root.tabText(root.currentTab)

                Keys.priority: Keys.BeforeItem
                // the current card holds the focus inside the view, so the
                // usual focus chain would come back to the view: say where
                // Tab and Shift+Tab go
                Keys.onPressed: event => {
                    root.keyboardUsed = true
                    event.accepted = false
                    if (event.key === Qt.Key_Backtab
                            || (event.key === Qt.Key_Tab && (event.modifiers & Qt.ShiftModifier))) {
                        (tabs.visible ? tabs : cancelButton).forceActiveFocus(Qt.BacktabFocusReason)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Tab) {
                        root.firstFooterControl().forceActiveFocus(Qt.TabFocusReason)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Space) {
                        root.choose(currentIndex)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        root.enterOn(currentIndex)
                        event.accepted = true
                    }
                }

                delegate: Item {
                    id: cell

                    required property int index
                    required property var model

                    readonly property string kind: root.currentTab === "window" ? "window"
                                                 : root.kindOfOutput(root.sourceRow(index))
                    readonly property var pixels: kind === "screen" && model.geometry
                        ? Logic.pixelSize(model.geometry.width, model.geometry.height,
                                          model.screen ? model.screen.devicePixelRatio : 1)
                        : null

                    width: grid.cellWidth
                    height: grid.cellHeight

                    SourceCard {
                        objectName: "card"
                        anchors.horizontalCenter: parent.horizontalCenter
                        y: Tokens.s1
                        kind: cell.kind
                        multiple: root.multiple
                        previewComponent: root.previewComponent
                        selected: cell.model.checked === Qt.Checked
                        current: grid.currentIndex === cell.index
                        showFocus: grid.activeFocus && root.keyboardUsed
                        iconSource: cell.model.decoration ?? ""
                        outputName: cell.model.name ?? ""
                        uuid: cell.model.Uuid ?? ""
                        number: Logic.screenNumber(cell.index)
                        label: {
                            switch (cell.kind) {
                            case "screen":
                                return cell.pixels
                                    ? i18nc("@label %1 is the screen number, %2 × %3 its size in pixels",
                                            "Screen %1 · %2 × %3", String(Logic.screenNumber(cell.index)),
                                            String(cell.pixels.width), String(cell.pixels.height))
                                    : i18nc("@label %1 is the screen number", "Screen %1",
                                            String(Logic.screenNumber(cell.index)))
                            case "window":
                                return Logic.windowTitle(cell.model.display)
                                       || i18nc("@label a window without a title", "Untitled window")
                            case "workspace":
                                return i18nc("@label share every screen at once", "All screens")
                            case "virtual":
                                return i18nc("@label", "New virtual screen")
                            case "region":
                                return i18nc("@label share a part of the screen", "Select an area")
                            }
                            return cell.model.display ?? ""
                        }
                        hint: {
                            switch (cell.kind) {
                            case "workspace":
                                return i18nc("@info", "Everything on all your screens")
                            case "virtual":
                                return i18nc("@info", "An extra screen, just for sharing")
                            case "region":
                                return i18nc("@info", "After Share, drag over the part of your screens to share.")
                            }
                            return ""
                        }
                        onClicked: {
                            grid.currentIndex = cell.index
                            root.choose(cell.index)
                        }
                        onDoubleClicked: {
                            grid.currentIndex = cell.index
                            root.chooseAndShare(cell.index)
                        }
                    }
                }

                T.ScrollBar.vertical: T.ScrollBar {
                    id: scrollBar
                    parent: gridArea
                    x: gridArea.width - width - Tokens.s1
                    y: grid.y
                    height: grid.height
                    width: hovered || pressed ? Tokens.sizeScrollbarExpanded : Tokens.sizeScrollbarThumb + 2
                    policy: grid.count > 0 && grid.contentHeight > grid.height ? T.ScrollBar.AlwaysOn : T.ScrollBar.AlwaysOff
                    visible: policy === T.ScrollBar.AlwaysOn
                    hoverEnabled: true
                    padding: 2
                    contentItem: Rectangle {
                        implicitWidth: Tokens.sizeScrollbarThumb
                        radius: width / 2
                        color: Tokens.strokeStrong
                        opacity: scrollBar.pressed ? 1 : scrollBar.hovered ? 0.9 : 0.6
                    }
                    Behavior on width {
                        NumberAnimation { duration: Tokens.durationFaster }
                    }
                }
            }

            // the cards that go on below fade out instead of being cut
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: Tokens.s5
                visible: !grid.atYEnd
                gradient: Gradient {
                    GradientStop { position: 0; color: Qt.alpha(root.color, 0) }
                    GradientStop { position: 1; color: root.color }
                }
            }

            // nothing to show in this tab
            Column {
                objectName: "emptyState"
                anchors.centerIn: parent
                anchors.verticalCenterOffset: -Tokens.s5
                visible: grid.count === 0
                spacing: Tokens.s3
                width: parent.width - 2 * Tokens.s10

                Kirigami.Icon {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Tokens.sizeIconTile
                    height: width
                    source: "window-new-symbolic"
                    isMask: true
                    color: Tokens.textTertiary
                }
                Text {
                    textFormat: Text.PlainText
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: root.currentTab === "window"
                          ? i18nc("@info", "No windows to share")
                          : i18nc("@info", "Nothing to share here")
                    font: Tokens.bodyStrong
                    color: Tokens.textPrimary
                    wrapMode: Text.Wrap
                }
                Text {
                    textFormat: Text.PlainText
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    visible: root.currentTab === "window"
                    text: i18nc("@info", "Open the window you want to share and it shows up here.")
                    font: Tokens.body
                    color: Tokens.textSecondary
                    wrapMode: Text.Wrap
                }
            }
        }

        // --- options and buttons ------------------------------------------
        Rectangle {
            id: footer
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: Math.max(80, options.implicitHeight + 2 * Tokens.s5)
            color: Tokens.bgSecondary

            Rectangle {
                width: parent.width
                height: 1
                color: Tokens.strokeDivider
            }

            Column {
                id: options
                anchors.left: parent.left
                anchors.leftMargin: root.margin
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: buttons.left
                anchors.rightMargin: Tokens.s5
                spacing: 0

                Row {
                    objectName: "pointerRow"
                    visible: Logic.offersPointer(root.cursorMode)
                    spacing: Tokens.s4
                    height: Tokens.sizeControlHeight

                    Switch {
                        id: pointerOption
                        objectName: "pointerOption"
                        anchors.verticalCenter: parent.verticalCenter
                        checked: true
                        showStateText: false
                        text: pointerLabel.text
                        activeFocusOnTab: parent.visible
                    }
                    Text {
                        textFormat: Text.PlainText
                        id: pointerLabel
                        anchors.verticalCenter: parent.verticalCenter
                        text: i18nc("@option:check", "Share the pointer")
                        font: Tokens.body
                        color: Tokens.textPrimary
                        MouseArea {
                            anchors.fill: parent
                            onClicked: pointerOption.toggle()
                        }
                    }
                }

                CheckOption {
                    id: rememberOption
                    objectName: "rememberOption"
                    visible: root.persistRequested
                    width: Math.min(implicitWidth, parent.width)
                    checked: false
                    keyboardUsed: root.keyboardUsed
                    activeFocusOnTab: visible
                    text: i18nc("@option:check %1 is the name of an app",
                                "Remember this choice for %1", root.appName)
                }
            }

            Row {
                id: buttons
                anchors.right: parent.right
                anchors.rightMargin: root.margin
                anchors.verticalCenter: parent.verticalCenter
                spacing: Tokens.s3

                AccentButton {
                    id: shareButton
                    objectName: "shareButton"
                    width: 120
                    text: i18nc("@action:button", "Share")
                    enabled: root.canShare
                    onClicked: root.share()
                }
                AccentButton {
                    id: cancelButton
                    objectName: "cancelButton"
                    width: 120
                    accent: false
                    text: i18nc("@action:button", "Cancel")
                    onClicked: root.cancel()
                }
            }
        }
    }

    // a click anywhere means the pointer is in use: no focus rings
    MouseArea {
        anchors.fill: parent
        z: 1000
        acceptedButtons: Qt.AllButtons
        onPressed: mouse => {
            root.keyboardUsed = false
            mouse.accepted = false
        }
    }
}

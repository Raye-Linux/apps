// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

// The tabs above the grid ("Entire screen", "Window", "Area"): one rounded
// control, the chosen segment raised with a short accent pill under its
// text. The whole control is one Tab stop; Left and Right change the tab.
import QtQuick
import org.raye.ui

import "PickerLogic.js" as Logic

FocusScope {
    id: control

    // [{ id: "screen", text: "Entire screen" }, ...]
    property var tabs: []
    property string current: tabs.length > 0 ? tabs[0].id : ""
    // set by the picker when the last input was the keyboard
    property bool keyboardUsed: false

    signal activated(string id)

    readonly property var ids: tabs.map(t => t.id)

    implicitWidth: row.implicitWidth + 2 * Tokens.s1
    implicitHeight: Tokens.sizeControlHeight + 4
    activeFocusOnTab: true

    Accessible.role: Accessible.PageTabList

    function choose(id) {
        if (id === "" || id === current)
            return
        current = id
        activated(id)
    }

    Keys.onLeftPressed: event => { choose(Logic.nextTab(ids, current, -1)); event.accepted = true }
    Keys.onRightPressed: event => { choose(Logic.nextTab(ids, current, 1)); event.accepted = true }

    Rectangle {
        id: track
        anchors.fill: parent
        radius: Tokens.radiusOverlay - 2
        color: Tokens.fillControlPressed
        border.width: 1
        border.color: Tokens.strokeControl

        FocusRing {
            radius: track.radius
            shown: control.activeFocus && control.keyboardUsed
        }
    }

    Row {
        id: row
        anchors.verticalCenter: parent.verticalCenter
        x: Tokens.s1
        spacing: Tokens.s1

        Repeater {
            model: control.tabs

            delegate: Item {
                id: segment

                required property var modelData
                readonly property bool chosen: modelData.id === control.current

                width: Math.max(96, label.implicitWidth + 2 * Tokens.s5)
                height: Tokens.sizeControlHeight

                Accessible.role: Accessible.PageTab
                Accessible.name: modelData.text
                Accessible.selected: chosen
                Accessible.onPressAction: control.choose(modelData.id)

                Rectangle {
                    anchors.fill: parent
                    radius: Tokens.radiusControl + 1
                    color: segment.chosen ? Tokens.bgCardSolid
                         : mouse.pressed ? Tokens.fillSubtlePressed
                         : mouse.containsMouse ? Tokens.fillSubtleHover : "transparent"
                    border.width: segment.chosen ? 1 : 0
                    border.color: Tokens.strokeControl

                    Behavior on color {
                        ColorAnimation { duration: Tokens.durationFaster }
                    }
                }

                Text {
                    textFormat: Text.PlainText
                    id: label
                    anchors.centerIn: parent
                    anchors.verticalCenterOffset: -1
                    text: segment.modelData.text
                    font: segment.chosen ? Tokens.bodyStrong : Tokens.body
                    color: segment.chosen ? Tokens.textPrimary : Tokens.textSecondary
                }

                // the pill under the chosen tab, like Windows' selector bar
                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 3
                    width: segment.chosen ? 16 : 0
                    height: 3
                    radius: 1.5
                    color: Tokens.accent
                    opacity: segment.chosen ? 1 : 0

                    Behavior on width {
                        NumberAnimation { duration: Tokens.durationFast; easing.type: Tokens.easingFast }
                    }
                }

                MouseArea {
                    id: mouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        control.forceActiveFocus(Qt.MouseFocusReason)
                        control.choose(segment.modelData.id)
                    }
                }
            }
        }
    }
}

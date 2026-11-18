// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Finding a city for the world clock: type, then pick it from the list.
// With nothing typed, the big cities.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

Sheet {
    id: sheet

    property var clock
    property var results: []

    function openSearch() {
        search.text = ""
        results = clock.searchCities("")
        open()
        search.forceActiveFocus()
    }
    function pick(key) {
        clock.addCity(key)
        close()
    }

    heading: i18nc("@title:window", "Add a new city")
    onAccepted: if (results.length > 0) pick(results[0].key)

    TextBox {
        id: search
        Layout.fillWidth: true
        leftPadding: Tokens.s4 + 16 + Tokens.s3
        placeholderText: i18nc("@info:placeholder", "Enter a city or a country")
        onTextChanged: sheet.results = sheet.clock.searchCities(text)
        Keys.onDownPressed: list.forceActiveFocus()
        Glyph {
            anchors.left: parent.left
            anchors.leftMargin: Tokens.s4
            anchors.verticalCenter: parent.verticalCenter
            kind: "search"
            color: Tokens.textSecondary
        }
    }

    ListView {
        id: list
        Layout.fillWidth: true
        Layout.preferredHeight: 320
        clip: true
        model: sheet.results
        boundsBehavior: Flickable.StopAtBounds
        keyNavigationEnabled: true
        highlightMoveDuration: 0
        QQC2.ScrollBar.vertical: Scroller {}
        WheelScroll { target: list }
        Keys.onReturnPressed: if (currentIndex >= 0) sheet.pick(sheet.results[currentIndex].key)

        delegate: QQC2.AbstractButton {
            id: item
            required property var modelData
            required property int index
            width: ListView.view.width
            height: 48
            hoverEnabled: true
            Accessible.name: modelData.name + ", " + modelData.country
            onClicked: sheet.pick(modelData.key)
            background: Rectangle {
                radius: Tokens.radiusControl
                color: item.pressed ? Tokens.fillSubtlePressed
                     : item.hovered || (list.activeFocus && list.currentIndex === item.index) ? Tokens.fillSubtleHover
                     : "transparent"
            }
            contentItem: RowLayout {
                spacing: Tokens.s4
                Glyph {
                    Layout.leftMargin: Tokens.s3
                    kind: "globe"
                    color: Tokens.textSecondary
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    QQC2.Label {
                        Layout.fillWidth: true
                        text: item.modelData.name
                        color: Tokens.textPrimary
                        font: Tokens.body
                        elide: Text.ElideRight
                    }
                    QQC2.Label {
                        Layout.fillWidth: true
                        visible: item.modelData.country !== ""
                        text: item.modelData.country
                        color: Tokens.textSecondary
                        font: Tokens.caption
                        elide: Text.ElideRight
                    }
                }
            }
        }

        QQC2.Label {
            anchors.centerIn: parent
            visible: sheet.results.length === 0
            text: i18nc("@info", "No city found")
            color: Tokens.textSecondary
            font: Tokens.body
        }
    }

    buttons: [
        Item { Layout.fillWidth: true },
        AccentButton {
            Layout.preferredWidth: 120
            accent: false
            text: i18nc("@action:button", "Cancel")
            onClicked: sheet.close()
        }
    ]
}

// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// One category of the home page: its picture, its name, and its two or
// three most asked-for tasks under it.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.raye.ui

RowLayout {
    id: block

    required property var modelData
    // the links this PC can open
    property var links: []

    signal categoryOpened(string id)
    signal linkOpened(var link)

    spacing: Tokens.s5

    Kirigami.Icon {
        Layout.alignment: Qt.AlignTop
        Layout.preferredWidth: Tokens.sizeIconTile
        Layout.preferredHeight: Tokens.sizeIconTile
        source: block.modelData.icon
        fallback: "preferences-other"
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        spacing: Tokens.s2

        // lets the column take its share of the row; the links keep
        // their own width so only the words are clickable
        Item { Layout.fillWidth: true; Layout.preferredHeight: 0 }

        LinkText {
            Layout.fillWidth: true
            Layout.maximumWidth: implicitWidth
            text: block.modelData.name
            textFont: Tokens.subtitle
            plain: true
            strong: true
            onClicked: block.categoryOpened(block.modelData.id)
        }

        Repeater {
            model: block.links
            delegate: LinkText {
                required property var modelData
                Layout.fillWidth: true
            Layout.maximumWidth: implicitWidth
                text: modelData.text
                onClicked: block.linkOpened(modelData)
            }
        }
    }
}

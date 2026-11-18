// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// One reading of the Overview page: its name, the figure, a line under it
// and the last minute as a graph.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

Card {
    id: tile

    property string title
    property string value
    property string detail
    property var values: []
    property real maximum: 100
    property color color: Tokens.accent

    implicitHeight: 148

    ColumnLayout {
        anchors.fill: parent
        spacing: Tokens.s1

        QQC2.Label {
            Layout.fillWidth: true
            text: tile.title
            color: Tokens.textSecondary
            font: Tokens.body
            elide: Text.ElideRight
        }
        QQC2.Label {
            Layout.fillWidth: true
            text: tile.value
            color: Tokens.textPrimary
            font: Tokens.title
            elide: Text.ElideRight
        }
        QQC2.Label {
            Layout.fillWidth: true
            text: tile.detail
            visible: text !== ""
            color: Tokens.textTertiary
            font: Tokens.caption
            elide: Text.ElideRight
        }
        Item { Layout.fillHeight: true }
        Graph {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            values: tile.values
            maximum: tile.maximum
            color: tile.color
            grid: false
            framed: false
        }
    }
}

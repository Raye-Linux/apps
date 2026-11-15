// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// An item on a category page or in the search results: its picture, its
// name, and its tasks as links under it, the way the classic Control
// Panel shows them.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.raye.ui

RowLayout {
    id: block

    property string name
    property string summary
    property string iconName
    property string glyph
    // tasks that can be opened on this PC
    property var tasks: []
    property bool showSummary: false

    signal opened()
    signal taskOpened(var task)

    spacing: Tokens.s5

    Item {
        Layout.alignment: Qt.AlignTop
        Layout.preferredWidth: Tokens.sizeIconTile
        Layout.preferredHeight: Tokens.sizeIconTile
        Kirigami.Icon {
            anchors.fill: parent
            visible: block.glyph === ""
            source: block.iconName
            fallback: "preferences-other"
        }
        Rectangle {
            anchors.fill: parent
            visible: block.glyph !== ""
            radius: Tokens.radiusOverlay
            color: Tokens.bgCard
            border.width: 1
            border.color: Tokens.strokeCard
            Glyph {
                anchors.centerIn: parent
                width: 22
                height: 22
                kind: block.glyph || "list"
                color: Tokens.textSecondary
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        spacing: Tokens.s1

        LinkText {
            Layout.fillWidth: true
            Layout.maximumWidth: implicitWidth
            text: block.name
            plain: true
            strong: true
            textFont: Tokens.bodyLarge
            QQC2.ToolTip.visible: hovered && block.summary !== "" && !block.showSummary
            QQC2.ToolTip.delay: 700
            QQC2.ToolTip.text: block.summary
            onClicked: block.opened()
        }

        QQC2.Label {
            Layout.fillWidth: true
            visible: block.showSummary && block.summary !== ""
            text: block.summary
            wrapMode: Text.Wrap
            color: Tokens.textSecondary
            font: Tokens.body
        }

        Flow {
            Layout.fillWidth: true
            visible: block.tasks.length > 0
            spacing: Tokens.s1
            Repeater {
                model: block.tasks
                delegate: Row {
                    required property var modelData
                    required property int index
                    spacing: Tokens.s3
                    LinkText {
                        text: modelData.text
                        onClicked: block.taskOpened(modelData)
                    }
                    // a thin bar between two tasks on one line
                    Rectangle {
                        visible: index < block.tasks.length - 1
                        anchors.verticalCenter: parent.verticalCenter
                        width: 1
                        height: 12
                        color: Tokens.strokeDivider
                    }
                    Item { width: Tokens.s1; height: 1 }
                }
            }
        }
    }
}

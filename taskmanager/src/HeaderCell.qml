// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// A column heading. Pressing it sorts by the column, and pressing it
// again turns the order round. total, when set, is the machine's figure
// drawn above the word, as Windows does on the Processes page.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

QQC2.AbstractButton {
    id: cell

    property string column
    property string sortColumn
    property bool sortDescending
    property string total
    property bool alignRight: true
    property bool busy: false // the total is high: drawn in the caution colour

    signal sortRequested(string column, bool descending)

    readonly property bool sorted: column !== "" && sortColumn === column
    implicitHeight: Tokens.sizeListItem + (total !== "" ? Tokens.s5 : 0)
    hoverEnabled: true
    Accessible.name: text

    onClicked: {
        // figures read best biggest first; names in the alphabet's order
        const first = column !== "name" && column !== "user"
        sortRequested(column, sorted ? !sortDescending : first)
    }

    background: Rectangle {
        color: cell.pressed ? Tokens.fillSubtlePressed : cell.hovered ? Tokens.fillSubtleHover : "transparent"
        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.topMargin: Tokens.s3
            anchors.bottomMargin: Tokens.s3
            width: 1
            color: Tokens.strokeDivider
        }
    }

    contentItem: ColumnLayout {
        spacing: 0
        QQC2.Label {
            Layout.fillWidth: true
            visible: cell.total !== ""
            horizontalAlignment: cell.alignRight ? Text.AlignRight : Text.AlignLeft
            text: cell.total
            color: cell.busy ? Tokens.statusCaution : Tokens.textPrimary
            font: Tokens.bodyStrong
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: Tokens.s1
            Item { Layout.fillWidth: cell.alignRight }
            QQC2.Label {
                Layout.fillWidth: !cell.alignRight
                elide: Text.ElideRight
                text: cell.text
                color: cell.sorted ? Tokens.textPrimary : Tokens.textSecondary
                font: Tokens.caption
            }
            Glyph {
                Layout.preferredWidth: 12
                Layout.preferredHeight: 12
                visible: cell.sorted
                kind: cell.sortDescending ? "sortDown" : "sortUp"
                color: Tokens.textSecondary
            }
        }
    }
    leftPadding: Tokens.s3
    rightPadding: Tokens.s3
}

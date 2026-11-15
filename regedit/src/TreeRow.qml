// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// One key of the tree: the arrow, a folder, the name.

import QtQuick
import QtQuick.Controls as QQC2
import org.raye.ui

Item {
    id: delegateRoot

    required property TreeView treeView
    required property bool isTreeNode
    required property bool expanded
    required property bool hasChildren
    required property int depth
    required property int row
    required property bool current
    required property string name
    required property string kind

    signal chosen()
    signal menuAsked(real x, real y)

    implicitWidth: label.x + label.implicitWidth + Tokens.s4
    implicitHeight: Tokens.sizeTreeItem - 4

    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: Tokens.s1
        anchors.rightMargin: Tokens.s1
        radius: Tokens.radiusControl
        color: delegateRoot.current ? Tokens.fillSubtleSelected : hover.hovered ? Tokens.fillSubtleHover : "transparent"

        Rectangle {
            visible: delegateRoot.current
            x: 0
            anchors.verticalCenter: parent.verticalCenter
            width: Tokens.sizePillWidth
            height: Tokens.sizePillHeight
            radius: Tokens.radiusNavPill
            color: Tokens.accent
        }
    }

    HoverHandler { id: hover }

    TapHandler {
        acceptedButtons: Qt.LeftButton
        onTapped: delegateRoot.chosen()
        onDoubleTapped: if (delegateRoot.hasChildren) delegateRoot.treeView.toggleExpanded(delegateRoot.row)
    }
    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: eventPoint => {
            delegateRoot.chosen()
            delegateRoot.menuAsked(eventPoint.position.x, eventPoint.position.y)
        }
    }

    Glyph {
        id: arrow
        x: Tokens.s3 + delegateRoot.depth * 16
        anchors.verticalCenter: parent.verticalCenter
        visible: delegateRoot.hasChildren
        kind: delegateRoot.expanded ? "chevronDown" : "chevron"
        color: Tokens.textSecondary
        width: 12
        height: 12

        TapHandler {
            onTapped: delegateRoot.treeView.toggleExpanded(delegateRoot.row)
        }
    }

    Glyph {
        id: icon
        x: arrow.x + 18
        anchors.verticalCenter: parent.verticalCenter
        kind: delegateRoot.kind === "computer" ? "computer" : delegateRoot.kind === "file" ? "file"
            : delegateRoot.kind === "lockedfolder" ? "lock" : delegateRoot.expanded ? "folderOpen" : "folder"
        color: delegateRoot.kind === "file" ? Tokens.textSecondary : Tokens.accentText
    }

    QQC2.Label {
        id: label
        anchors.left: icon.right
        anchors.leftMargin: Tokens.s3
        anchors.verticalCenter: parent.verticalCenter
        text: delegateRoot.name
        color: Tokens.textPrimary
        font: delegateRoot.current ? Tokens.bodyStrong : Tokens.body
    }
}

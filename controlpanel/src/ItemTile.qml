// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// One item of the icon views: large (picture over two lines of words
// beside it) or small (one line).

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.raye.ui

QQC2.AbstractButton {
    id: tile

    property string iconName
    property string summary
    property bool large: true
    // its program is starting
    property bool starting: false

    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    implicitHeight: large ? 56 : Tokens.sizeTreeItem
    Accessible.name: text
    Accessible.description: summary
    QQC2.ToolTip.visible: hovered && summary !== ""
    QQC2.ToolTip.delay: 700
    QQC2.ToolTip.text: summary

    background: Rectangle {
        radius: Tokens.radiusControl
        color: tile.pressed || tile.starting ? Tokens.fillSubtlePressed : tile.hovered ? Tokens.fillSubtleHover : "transparent"
        FocusRing {
            anchors.fill: parent
            visible: tile.visualFocus
        }
    }

    contentItem: RowLayout {
        spacing: tile.large ? Tokens.s4 : Tokens.s3
        Kirigami.Icon {
            Layout.leftMargin: Tokens.s3
            Layout.preferredWidth: tile.large ? Tokens.sizeIconMultiline : Tokens.sizeIconList
            Layout.preferredHeight: Layout.preferredWidth
            source: tile.iconName
            fallback: "preferences-other"
        }
        QQC2.Label {
            Layout.fillWidth: true
            text: tile.text
            elide: Text.ElideRight
            color: Tokens.textPrimary
            font: Tokens.body
        }
    }
}

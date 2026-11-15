// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// One row of the Processes page: a section heading, an app with the
// number of its processes, one of those processes, or a background
// process. The widths come from the page, which lays the headings out
// with the same ones.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.raye.ui

Item {
    id: row

    required property string kind
    required property int section
    required property string key
    required property string name
    required property string icon
    required property int pid
    required property int count
    required property real cpu
    required property double memory
    required property real disk
    required property bool efficiency
    required property string state
    required property bool expanded

    property bool selected: false
    property int statusWidth: 120
    property int figureWidth: 88

    signal clicked()
    signal toggled()
    signal menuRequested(real x, real y)

    readonly property bool heading: kind === "header"
    implicitHeight: heading ? Tokens.sizeListItem : Tokens.sizeTreeItem

    Rectangle {
        anchors.fill: parent
        anchors.topMargin: 1
        anchors.bottomMargin: 1
        visible: !row.heading
        radius: Tokens.radiusControl
        color: row.selected ? Tokens.fillSubtleSelected : hover.hovered ? Tokens.fillSubtleHover : "transparent"
        Behavior on color { ColorAnimation { duration: Tokens.durationFaster } }
    }

    HoverHandler { id: hover; enabled: !row.heading }
    TapHandler {
        enabled: !row.heading
        acceptedButtons: Qt.LeftButton
        onTapped: row.clicked()
        onDoubleTapped: if (row.kind === "app" && row.count > 1) row.toggled()
    }
    TapHandler {
        enabled: !row.heading
        acceptedButtons: Qt.RightButton
        onTapped: (point) => { row.clicked(); row.menuRequested(point.position.x, point.position.y) }
    }

    QQC2.Label {
        visible: row.heading
        anchors.left: parent.left
        anchors.leftMargin: Tokens.s3
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Tokens.s2
        text: row.section === 0
              ? i18nc("@title the apps section of the process list, with how many", "Apps (%1)", row.count)
              : i18nc("@title the background section of the process list, with how many", "Background processes (%1)", row.count)
        color: Tokens.textPrimary
        font: Tokens.bodyStrong
    }

    RowLayout {
        visible: !row.heading
        anchors.fill: parent
        anchors.leftMargin: Tokens.s2
        spacing: 0

        // the arrow of a group, or the room it would take
        Item {
            Layout.preferredWidth: 20 + (row.kind === "child" ? 24 : 0)
            Layout.fillHeight: true
            Glyph {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: 12
                height: 12
                visible: row.kind === "app" && row.count > 1
                kind: row.expanded ? "chevronDown" : "chevron"
                color: Tokens.textSecondary
                TapHandler { onTapped: row.toggled() }
            }
        }

        Kirigami.Icon {
            Layout.leftMargin: Tokens.s3
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            source: row.icon !== "" ? row.icon : "application-x-executable"
            fallback: "application-x-executable"
        }

        QQC2.Label {
            Layout.fillWidth: true
            Layout.leftMargin: Tokens.s3
            elide: Text.ElideRight
            text: row.kind === "app" && row.count > 1 ? i18nc("@info an app and how many processes it has", "%1 (%2)", row.name, row.count) : row.name
            color: Tokens.textPrimary
            font: Tokens.body
        }

        // what Windows puts in Status: a leaf for the efficiency mode,
        // and "Suspended" for a stopped process
        RowLayout {
            Layout.preferredWidth: row.statusWidth
            Layout.maximumWidth: row.statusWidth
            spacing: Tokens.s2
            Item { Layout.fillWidth: true }
            Glyph {
                visible: row.efficiency
                kind: "leaf"
                color: Tokens.statusSuccess
            }
            QQC2.Label {
                elide: Text.ElideRight
                text: row.state === "T" || row.state === "t" ? i18nc("@info:status a process stopped by a signal", "Suspended")
                      : row.efficiency ? i18nc("@info:status marks a process in the efficiency mode", "Efficiency mode") : ""
                color: Tokens.textSecondary
                font: Tokens.caption
            }
        }

        Figure {
            width: row.figureWidth
            text: Fmt.percent(row.cpu)
            heat: row.cpu / 25
        }
        Figure {
            width: row.figureWidth
            text: Fmt.bytes(row.memory)
            heat: row.memory / 2147483648
        }
        Figure {
            width: row.figureWidth
            // another user's process: its disk use cannot be read
            text: row.disk < 0 ? "—" : Fmt.rate(row.disk)
            heat: row.disk / 10485760
        }
    }

    // a figure cell, tinted like Windows' heat map: the busier, the deeper
    component Figure: Rectangle {
        property alias text: label.text
        property real heat: 0
        Layout.preferredWidth: width
        Layout.fillHeight: true
        Layout.topMargin: 1
        Layout.bottomMargin: 1
        color: Qt.alpha(Tokens.accent, Math.min(0.32, Math.max(0, heat) * 0.32))
        QQC2.Label {
            id: label
            anchors.fill: parent
            anchors.rightMargin: Tokens.s3
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter
            color: Tokens.textPrimary
            font: Tokens.body
        }
    }
}

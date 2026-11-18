// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// A dialog over the window: a title, what goes in it, and the buttons
// along the bottom on the darker footer band, as Windows draws them.
// Return presses the first (accent) button; Esc closes.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

QQC2.Popup {
    id: sheet

    property string heading
    default property alias content: body.data
    // the buttons, left to right
    property alias buttons: buttonRow.data
    signal accepted()

    parent: QQC2.Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(440, parent ? parent.width - 2 * Tokens.s7 : 440)
    modal: true
    focus: true
    padding: 0
    closePolicy: QQC2.Popup.CloseOnEscape

    QQC2.Overlay.modal: Rectangle {
        color: Tokens.bgSmoke
        Behavior on opacity { NumberAnimation { duration: Tokens.durationFast } }
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Tokens.durationNormal; easing.type: Tokens.easingNormal }
            NumberAnimation { property: "scale"; from: 1.05; to: 1; duration: Tokens.durationNormal; easing.type: Tokens.easingNormal }
        }
    }
    exit: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; to: 0; duration: Tokens.durationExit; easing.type: Tokens.easingExit }
            NumberAnimation { property: "scale"; to: 1.02; duration: Tokens.durationExit; easing.type: Tokens.easingExit }
        }
    }

    background: Rectangle {
        radius: Tokens.radiusOverlay
        color: Tokens.bgTertiary
        border.width: 1
        border.color: Tokens.strokeSurface
    }

    contentItem: ColumnLayout {
        spacing: 0
        Keys.onReturnPressed: sheet.accepted()
        Keys.onEnterPressed: sheet.accepted()

        QQC2.Label {
            Layout.fillWidth: true
            Layout.margins: Tokens.s7
            Layout.bottomMargin: Tokens.s5
            text: sheet.heading
            color: Tokens.textPrimary
            font: Tokens.subtitle
            elide: Text.ElideRight
        }
        ColumnLayout {
            id: body
            Layout.fillWidth: true
            Layout.leftMargin: Tokens.s7
            Layout.rightMargin: Tokens.s7
            Layout.bottomMargin: Tokens.s7
            spacing: Tokens.s5
        }
        // one pixel in, so the dialog's edge runs around it
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 1
            Layout.rightMargin: 1
            Layout.bottomMargin: 1
            implicitHeight: 80
            color: Tokens.bgSecondary
            radius: Tokens.radiusOverlay - 1
            // square top corners: only the bottom of the dialog is round
            Rectangle {
                width: parent.width
                height: parent.radius
                color: parent.color
            }
            Rectangle {
                width: parent.width
                height: 1
                color: Tokens.strokeDivider
            }
            RowLayout {
                id: buttonRow
                anchors.fill: parent
                anchors.margins: Tokens.s7
                spacing: Tokens.s3
            }
        }
    }
}

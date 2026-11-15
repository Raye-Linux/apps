// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The one question the window asks before it does something a user
// cannot undo. Two of them exist: forcing a program to close, and
// putting one in the efficiency mode, which on this system the kernel
// may refuse to reverse.
//
// The refusing button is the DEFAULT one: pressing Return closes the
// sheet without doing the thing.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

QQC2.Dialog {
    id: sheet

    property string heading
    property string body
    property string confirmText
    property bool destructive: false

    signal confirmed()

    modal: true
    anchors.centerIn: parent
    padding: Tokens.s4
    closePolicy: QQC2.Popup.CloseOnEscape

    QQC2.Overlay.modal: Rectangle { color: Tokens.bgSmoke }

    // a solid fill: the card colour is see-through, made for a card on the page
    background: Rectangle {
        radius: Tokens.radiusOverlay
        color: Tokens.bgTertiary
        border.width: 1
        border.color: Tokens.strokeSurface
    }

    contentItem: ColumnLayout {
        spacing: Tokens.s3

        QQC2.Label {
            Layout.fillWidth: true
            Layout.maximumWidth: Tokens.sizeTooltipMaxWidth * 2
            text: sheet.heading
            wrapMode: Text.Wrap
            color: Tokens.textPrimary
            font: Tokens.subtitle
        }

        QQC2.Label {
            Layout.fillWidth: true
            Layout.maximumWidth: Tokens.sizeTooltipMaxWidth * 2
            text: sheet.body
            wrapMode: Text.Wrap
            color: Tokens.textSecondary
            font: Tokens.body
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Tokens.s2
            spacing: Tokens.s2

            Item {
                Layout.fillWidth: true
            }

            AccentButton {
                accent: false
                focus: true
                text: i18nc("@action:button refuse what the sheet asks", "Cancel")
                onClicked: sheet.close()
            }

            AccentButton {
                text: sheet.confirmText
                onClicked: {
                    sheet.close()
                    sheet.confirmed()
                }
            }
        }
    }
}

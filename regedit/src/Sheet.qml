// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// A small dialog over the window: a heading, its content, and OK/Cancel.
// accepted() is only sent when okEnabled; Return presses OK.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

QQC2.Popup {
    id: sheet

    property string heading
    property string okText: i18nc("@action:button", "OK")
    property bool okEnabled: true
    property bool showCancel: true
    default property alias content: body.data

    signal accepted()

    modal: true
    anchors.centerIn: parent
    width: Math.min(460, parent.width - 2 * Tokens.s8)
    padding: Tokens.s7
    closePolicy: QQC2.Popup.CloseOnEscape

    function accept() {
        if (!okEnabled)
            return
        close()
        accepted()
    }

    QQC2.Overlay.modal: Rectangle { color: Tokens.bgSmoke }

    // a solid fill: a Card is see-through, made for a card on the page
    background: Rectangle {
        radius: Tokens.radiusOverlay
        color: Tokens.bgTertiary
        border.width: 1
        border.color: Tokens.strokeSurface
    }

    contentItem: ColumnLayout {
        spacing: Tokens.s5
        Keys.onReturnPressed: sheet.accept()
        Keys.onEnterPressed: sheet.accept()

        QQC2.Label {
            Layout.fillWidth: true
            text: sheet.heading
            wrapMode: Text.Wrap
            color: Tokens.textPrimary
            font: Tokens.subtitle
        }

        ColumnLayout {
            id: body
            Layout.fillWidth: true
            spacing: Tokens.s3
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Tokens.s3
            spacing: Tokens.s3
            Item { Layout.fillWidth: true }
            AccentButton {
                Layout.minimumWidth: 110
                text: sheet.okText
                enabled: sheet.okEnabled
                onClicked: sheet.accept()
            }
            AccentButton {
                Layout.minimumWidth: 110
                visible: sheet.showCancel
                accent: false
                text: i18nc("@action:button", "Cancel")
                onClicked: sheet.close()
            }
        }
    }
}

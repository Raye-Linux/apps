// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The foot of the Performance and Fan pages: what the last change did,
// Apply, and Restore defaults.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

ColumnLayout {
    id: bar

    property bool dirty: false
    property bool valid: true
    property bool busy: false
    property bool atDefaults: true
    property string message
    property bool messageOk: true

    signal applyClicked()
    signal discardClicked()
    signal restoreClicked()
    signal messageClosed()

    spacing: Tokens.s3

    // made only while there is something to say
    Loader {
        Layout.fillWidth: true
        active: bar.message !== ""
        visible: active
        sourceComponent: InfoBar {
            severity: bar.messageOk ? InfoBar.Success : InfoBar.Warning
            message: bar.message
            onCloseClicked: bar.messageClosed()
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Tokens.s3

        AccentButton {
            text: bar.busy ? i18nc("@action:button while it works", "Applying…") : i18nc("@action:button", "Apply")
            enabled: bar.dirty && bar.valid && !bar.busy
            onClicked: bar.applyClicked()
        }
        AccentButton {
            accent: false
            text: i18nc("@action:button", "Discard changes")
            visible: bar.dirty
            enabled: !bar.busy
            onClicked: bar.discardClicked()
        }
        Item { Layout.fillWidth: true }
        AccentButton {
            accent: false
            text: i18nc("@action:button", "Restore defaults")
            enabled: !bar.busy && !bar.atDefaults
            onClicked: bar.restoreClicked()
        }
    }
}

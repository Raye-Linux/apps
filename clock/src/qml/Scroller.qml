// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The scroll bar the Windows way: a thin line that widens under the pointer.

import QtQuick
import QtQuick.Controls as QQC2
import org.raye.ui

QQC2.ScrollBar {
    id: bar

    readonly property bool wide: hovered || pressed
    implicitWidth: Tokens.sizeScrollbarExpanded
    padding: 2
    hoverEnabled: true
    policy: size < 1 ? QQC2.ScrollBar.AsNeeded : QQC2.ScrollBar.AlwaysOff
    minimumSize: 0.08

    background: Rectangle {
        radius: width / 2
        color: Tokens.bgSecondary
        opacity: bar.wide ? 0.8 : 0
        Behavior on opacity { NumberAnimation { duration: Tokens.durationFast } }
    }
    contentItem: Item {
        implicitWidth: Tokens.sizeScrollbarThumb
        opacity: bar.active || bar.wide ? 1 : 0.35
        Behavior on opacity { NumberAnimation { duration: Tokens.durationNormal } }
        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: bar.wide ? Tokens.sizeScrollbarThumb : Tokens.sizeScrollbarThin
            radius: width / 2
            color: Tokens.strokeStrong
            Behavior on width { NumberAnimation { duration: Tokens.durationFast } }
        }
    }
}

// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// A date of the editor: the date in words, and a small month to pick another.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

QQC2.AbstractButton {
    id: field

    property date value: new Date()
    signal picked(date day)

    implicitHeight: Tokens.sizeControlHeight
    implicitWidth: 200
    hoverEnabled: true
    text: value.toLocaleDateString(Qt.locale(), i18nc("@info date format, QDate pattern", "ddd, MMM d, yyyy"))
    onClicked: popup.open()

    background: Rectangle {
        radius: Tokens.radiusControl
        color: field.pressed ? Tokens.fillControlPressed : field.hovered ? Tokens.fillControlHover : Tokens.fillControl
        border.width: 1
        border.color: Tokens.strokeControl
        FocusRing {
            anchors.fill: parent
            visible: field.visualFocus
        }
    }
    contentItem: RowLayout {
        QQC2.Label {
            Layout.fillWidth: true
            Layout.leftMargin: Tokens.s4
            text: field.text
            color: Tokens.textPrimary
            font: Tokens.body
        }
        Glyph {
            Layout.rightMargin: Tokens.s4
            kind: "today"
            color: Tokens.textSecondary
        }
    }

    QQC2.Popup {
        id: popup
        y: field.height + 2
        padding: Tokens.s3
        focus: true
        background: Rectangle {
            radius: Tokens.radiusOverlay
            color: Tokens.bgTertiary
            border.color: Tokens.strokeSurface
            border.width: 1
        }
        contentItem: MiniMonth {
            selected: field.value
            onPicked: day => {
                popup.close()
                field.picked(day)
            }
        }
    }
}

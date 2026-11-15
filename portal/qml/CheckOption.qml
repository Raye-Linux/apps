// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

// A check box the Windows way: a 20 px box with 4 px corners, filled with
// the accent and a white check when on, its text 8 px to the right.
import QtQuick
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami
import org.raye.ui

T.CheckBox {
    id: control

    property bool keyboardUsed: false

    implicitWidth: implicitIndicatorWidth + spacing + implicitContentWidth
    implicitHeight: Math.max(Tokens.sizeControlHeight, implicitIndicatorHeight)
    padding: 0
    spacing: Tokens.s3
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    font: Tokens.body

    Accessible.role: Accessible.CheckBox
    Accessible.name: text
    Accessible.checkable: true
    Accessible.checked: checked

    indicator: Rectangle {
        implicitWidth: Tokens.sizeCheckbox
        implicitHeight: Tokens.sizeCheckbox
        y: (control.height - height) / 2
        radius: Tokens.radiusControl
        color: control.checked ? (control.down ? Tokens.accentPressed
                                 : control.hovered ? Tokens.accentHover : Tokens.accent)
             : control.down ? Tokens.fillSubtlePressed
             : control.hovered ? Tokens.fillSubtleHover : Tokens.fillControlPressed
        border.width: control.checked ? 0 : 1
        border.color: Tokens.strokeStrong

        Behavior on color {
            ColorAnimation { duration: Tokens.durationFaster }
        }

        Kirigami.Icon {
            anchors.centerIn: parent
            width: Tokens.sizeIconMenu - 2
            height: width
            visible: control.checked
            source: "checkmark"
            isMask: true
            color: Tokens.textOnAccent
        }

        FocusRing {
            target: control
            radius: Tokens.radiusControl
            shown: control.activeFocus && control.keyboardUsed
        }
    }

    contentItem: Text {
        textFormat: Text.PlainText
        leftPadding: control.indicator.width + control.spacing
        text: control.text
        font: control.font
        color: Tokens.textPrimary
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}

// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// A slider the Windows way: a 4 px track filled with the accent up to a
// round thumb. A small tick marks the card's own value.

import QtQuick
import QtQuick.Templates as T
import org.raye.ui

T.Slider {
    id: control

    // the card's default, -1 for none
    property real mark: -1

    implicitWidth: 260
    implicitHeight: Tokens.sizeSliderThumb + Tokens.s3
    stepSize: 1
    snapMode: T.Slider.SnapAlways
    focusPolicy: Qt.StrongFocus
    hoverEnabled: true
    opacity: enabled ? 1 : 0.45

    background: Item {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.availableWidth
        height: Tokens.sizeSliderTrack

        Rectangle {
            anchors.fill: parent
            radius: height / 2
            color: Tokens.strokeStrong
            opacity: 0.6
        }
        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: height / 2
            color: Tokens.accent
        }
        Rectangle {
            visible: control.mark >= control.from && control.mark <= control.to
            x: (control.mark - control.from) / (control.to - control.from) * parent.width - width / 2
            y: parent.height + 4
            width: 2
            height: 6
            radius: 1
            color: Tokens.textTertiary
        }
    }

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: Tokens.sizeSliderThumb
        height: Tokens.sizeSliderThumb
        radius: width / 2
        color: Tokens.fillControlSolid
        border.width: 1
        border.color: Tokens.strokeControl

        Rectangle {
            anchors.centerIn: parent
            readonly property int size: control.pressed ? Tokens.sizeSliderThumbInnerPressed
                                      : control.hovered ? Tokens.sizeSliderThumbInnerHover : Tokens.sizeSliderThumbInner
            width: size
            height: size
            radius: size / 2
            color: Tokens.accent
            Behavior on width { NumberAnimation { duration: Tokens.durationFaster } }
        }
        FocusRing {
            anchors.fill: parent
            anchors.margins: -2
            visible: control.visualFocus
        }
    }
}

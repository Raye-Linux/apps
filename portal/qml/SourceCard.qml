// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

// One thing that can be shared: a 216 x 140 picture with 8 px corners and
// its name under it ("Screen 1 · 2560 × 1440", or the app icon and the
// window title). Chosen: a 2 px accent ring and a check badge. The
// picker decides what clicks and keys do; the card only draws.
import QtQuick
import org.kde.kirigami as Kirigami
import org.raye.ui

Item {
    id: card

    // "screen", "window", "workspace", "virtual", "region" or "other"
    property string kind: "screen"
    property string label
    property var iconSource
    property string outputName
    property string uuid
    property int number: 0
    property string hint
    property bool selected: false
    property bool current: false
    property bool showFocus: false
    property bool multiple: false
    // the picker's previewComponent: null shows the live picture
    property Component previewComponent: null

    signal clicked()
    signal doubleClicked()

    readonly property bool hasPicture: kind === "screen" || kind === "window"
    readonly property bool hovered: mouse.containsMouse

    width: 216
    height: 140 + Tokens.s3 + Tokens.bodyLineHeight

    Accessible.role: multiple ? Accessible.CheckBox : Accessible.RadioButton
    Accessible.name: label
    Accessible.description: hint
    Accessible.checkable: true
    Accessible.checked: selected
    Accessible.focusable: true
    Accessible.focused: current && showFocus
    Accessible.onPressAction: card.clicked()
    Accessible.onToggleAction: card.clicked()

    Rectangle {
        id: frame
        width: parent.width
        height: 140
        radius: Tokens.radiusOverlay
        color: mouse.pressed ? Tokens.fillControlPressedSolid
             : card.hovered ? Tokens.bgCardHoverSolid : Tokens.bgCardSolid
        border.width: 1
        border.color: Tokens.strokeCardSolid
        scale: mouse.pressed ? 0.98 : 1

        Behavior on color {
            ColorAnimation { duration: Tokens.durationFaster }
        }
        Behavior on scale {
            NumberAnimation { duration: Tokens.durationFaster; easing.type: Tokens.easingFast }
        }

        // the picture sits inside the card like on a mat
        Item {
            id: picture
            anchors.fill: parent
            anchors.margins: Tokens.s3
            visible: card.hasPicture
            clip: true

            Loader {
                id: preview
                anchors.fill: parent
                active: card.hasPicture
                onLoaded: {
                    item.kind = card.kind
                    item.outputName = card.outputName
                    item.uuid = card.uuid
                    item.number = card.number
                    item.title = card.label
                    item.iconSource = card.iconSource
                }
                Component.onCompleted: {
                    if (card.previewComponent)
                        sourceComponent = card.previewComponent
                    else
                        setSource(Qt.resolvedUrl("LiveThumbnail.qml"))
                }
            }
        }

        // no picture yet, or nothing to picture: the icon
        Kirigami.Icon {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: card.hasPicture || card.hint === "" ? 0 : -12
            width: card.hasPicture ? Tokens.sizeIconTile : Tokens.sizeIconMultiline + 8
            height: width
            visible: !card.hasPicture || !(preview.item && preview.item.live)
            source: card.kind === "workspace" ? "video-display-symbolic"
                  : card.kind === "virtual" ? "window-duplicate-symbolic"
                  : card.kind === "region" ? "transform-crop-symbolic"
                  : card.iconSource || "video-display-symbolic"
            // the symbolic icons take the text colour; app icons keep theirs
            isMask: !card.hasPicture
            color: Tokens.textSecondary
        }

        Text {
            textFormat: Text.PlainText
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: Tokens.s4
            visible: !card.hasPicture && card.hint !== ""
            text: card.hint
            font: Tokens.caption
            color: Tokens.textSecondary
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }

        // chosen: the accent ring, drawn over the edge
        Rectangle {
            anchors.fill: parent
            radius: frame.radius
            color: "transparent"
            border.width: 2
            border.color: Tokens.accent
            opacity: card.selected ? 1 : 0
            Behavior on opacity {
                NumberAnimation { duration: Tokens.durationFaster }
            }
        }

        // and the check badge in the top right corner
        Rectangle {
            id: badge
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: Tokens.s3
            width: Tokens.sizeIconList + 4
            height: width
            radius: width / 2
            color: Tokens.accent
            border.width: 2
            border.color: Tokens.bgCardSolid
            scale: card.selected ? 1 : 0.5
            opacity: card.selected ? 1 : 0
            Accessible.ignored: true

            Behavior on scale {
                NumberAnimation { duration: Tokens.durationFast; easing.type: Easing.OutBack }
            }
            Behavior on opacity {
                NumberAnimation { duration: Tokens.durationFaster }
            }

            Kirigami.Icon {
                anchors.centerIn: parent
                width: Tokens.sizeIconMenu - 2
                height: width
                source: "checkmark"
                isMask: true
                color: Tokens.textOnAccent
            }
        }

        FocusRing {
            radius: frame.radius
            shown: card.current && card.showFocus
        }
    }

    Row {
        id: caption
        anchors.top: frame.bottom
        anchors.topMargin: Tokens.s3
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(implicitWidth, parent.width)
        spacing: Tokens.s3

        Kirigami.Icon {
            id: appIcon
            anchors.verticalCenter: parent.verticalCenter
            width: Tokens.sizeIconMenu
            height: width
            visible: card.kind === "window"
            source: card.iconSource || "application-x-executable"
        }

        Text {
            objectName: "caption"
            textFormat: Text.PlainText
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(implicitWidth, card.width - (appIcon.visible ? appIcon.width + caption.spacing : 0))
            text: card.label
            font: card.selected ? Tokens.bodyStrong : Tokens.body
            color: Tokens.textPrimary
            elide: Text.ElideRight
            maximumLineCount: 1
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: card.clicked()
        onDoubleClicked: card.doubleClicked()
    }
}

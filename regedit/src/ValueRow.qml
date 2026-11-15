// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// One value of the selected key: name, type and data, in three columns.

import QtQuick
import QtQuick.Controls as QQC2
import org.raye.ui

Item {
    id: row

    required property int index
    required property string name
    required property string type
    required property string valueData
    required property bool locked
    required property string kind

    property bool selected: false
    property real nameWidth: 220
    property real typeWidth: 130

    signal chosen()
    signal opened()
    signal menuAsked(real x, real y)

    // what the type column says: words, not GVariant letters
    readonly property string shownType: {
        switch (kind) {
        case "Number": return i18nc("@item value type", "Number")
        case "YesNo": return i18nc("@item value type", "Yes/No")
        case "List": return i18nc("@item value type", "List")
        case "Text": return i18nc("@item value type", "Text")
        }
        switch (type) {
        case "s": return i18nc("@item value type", "Text")
        case "b": return i18nc("@item value type", "Yes/No")
        case "i": case "u": case "x": case "t": case "n": case "q": case "y": case "h":
            return i18nc("@item value type", "Number")
        case "d": return i18nc("@item value type", "Decimal number")
        case "as": return i18nc("@item value type", "List of text")
        }
        return i18nc("@item value type, %1 is a GVariant type such as (ii)", "Value (%1)", type)
    }
    readonly property string glyphKind: shownType === i18nc("@item value type", "Text") ? "text"
        : (kind === "Number" || kind === "YesNo") ? (kind === "YesNo" ? "yesno" : "number")
        : kind === "List" || type === "as" ? "list"
        : type === "b" ? "yesno" : "number"

    implicitHeight: Tokens.sizeTreeItem
    Accessible.role: Accessible.Row
    Accessible.name: name + ", " + shownType + ", " + valueData

    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: Tokens.s1
        anchors.rightMargin: Tokens.s1
        radius: Tokens.radiusControl
        color: row.selected ? Tokens.fillSubtleSelected : hover.hovered ? Tokens.fillSubtleHover : "transparent"
    }

    HoverHandler { id: hover }
    TapHandler {
        acceptedButtons: Qt.LeftButton
        onTapped: row.chosen()
        onDoubleTapped: row.opened()
    }
    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: eventPoint => {
            row.chosen()
            row.menuAsked(eventPoint.position.x, eventPoint.position.y)
        }
    }

    Glyph {
        id: icon
        x: Tokens.s4
        anchors.verticalCenter: parent.verticalCenter
        kind: row.locked ? "lock" : row.glyphKind
        color: row.locked ? Tokens.textSecondary : Tokens.accentText
    }
    QQC2.Label {
        x: icon.x + 16 + Tokens.s3
        width: row.nameWidth - x - Tokens.s3
        anchors.verticalCenter: parent.verticalCenter
        elide: Text.ElideRight
        text: row.name
        color: Tokens.textPrimary
        font: Tokens.body
    }
    QQC2.Label {
        x: row.nameWidth
        width: row.typeWidth - Tokens.s3
        anchors.verticalCenter: parent.verticalCenter
        elide: Text.ElideRight
        text: row.shownType
        color: Tokens.textSecondary
        font: Tokens.body
    }
    QQC2.Label {
        x: row.nameWidth + row.typeWidth
        width: row.width - x - Tokens.s4
        anchors.verticalCenter: parent.verticalCenter
        elide: Text.ElideRight
        maximumLineCount: 1
        text: row.valueData.split("\n").join(" ⏎ ")
        color: Tokens.textPrimary
        font: Tokens.body
    }
}

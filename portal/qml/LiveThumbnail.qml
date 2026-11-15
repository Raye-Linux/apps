// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

// The live picture of a screen or a window on a card: KWin streams it
// through PipeWire for as long as the card exists. Until the first frame
// comes (or when KWin gives none) the card shows its icon.
//
// Loaded by SourceCard only when the picker has no previewComponent, so
// the tests can show the picker without PipeWire or KWin.
import QtQuick
import org.kde.pipewire as PipeWire
import org.kde.taskmanager as TaskManager

Item {
    id: thumbnail

    // what the card shows: "screen" (outputName set) or "window" (uuid set)
    property string kind
    property string outputName
    property string uuid
    property int number
    property string title
    property var iconSource

    readonly property bool live: request.nodeId !== 0

    TaskManager.ScreencastingRequest {
        id: request
        outputName: thumbnail.kind === "screen" ? thumbnail.outputName : ""
        uuid: thumbnail.kind === "window" ? thumbnail.uuid : ""
    }

    PipeWire.PipeWireSourceItem {
        anchors.fill: parent
        nodeId: request.nodeId
        visible: thumbnail.live
    }
}

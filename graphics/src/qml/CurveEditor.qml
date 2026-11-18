// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The fan curve: temperature across, fan speed up. Each point is dragged
// with the pointer or moved with the arrow keys once chosen; it stays
// between its neighbours and inside what the card allows. A dashed line
// shows the card's temperature now.

import QtQuick
import QtQuick.Controls as QQC2
import org.raye.ui

Item {
    id: editor

    property var points: []          // [{ temp, speed }]
    property int tempMin: 20
    property int tempMax: 90
    property int speedMin: 0
    property int speedMax: 100
    property real now: -1            // °C, -1 when unknown
    property int selected: -1

    signal edited(var points)

    // the axes run a little past what the card allows
    readonly property int axisFrom: Math.floor(Math.min(tempMin, 20) / 10) * 10
    readonly property int axisTo: Math.min(110, Math.ceil((tempMax + 5) / 10) * 10)
    readonly property real padLeft: 44
    readonly property real padBottom: 28
    readonly property real plotW: width - padLeft - Tokens.s5
    readonly property real plotH: height - padBottom - Tokens.s3

    function xOf(t) { return padLeft + (t - axisFrom) / (axisTo - axisFrom) * plotW }
    function yOf(s) { return Tokens.s3 + (1 - s / 100) * plotH }
    function tOf(x) { return axisFrom + (x - padLeft) / plotW * (axisTo - axisFrom) }
    function sOf(y) { return (1 - (y - Tokens.s3) / plotH) * 100 }

    function moved(index, temp, speed) {
        const p = points.map(q => ({ temp: q.temp, speed: q.speed }))
        const lo = index > 0 ? p[index - 1].temp + 1 : tempMin
        const hi = index < p.length - 1 ? p[index + 1].temp - 1 : tempMax
        const slo = index > 0 ? Math.max(speedMin, p[index - 1].speed) : speedMin
        const shi = index < p.length - 1 ? Math.min(speedMax, p[index + 1].speed) : speedMax
        p[index] = { temp: Math.round(Math.max(lo, Math.min(hi, temp))),
                     speed: Math.round(Math.max(slo, Math.min(shi, speed))) }
        edited(p)
    }

    implicitHeight: 280
    activeFocusOnTab: true
    onPointsChanged: plot.requestPaint()
    onNowChanged: plot.requestPaint()
    onEnabledChanged: plot.requestPaint()
    Connections {
        target: Tokens
        function onNightChanged() { plot.requestPaint() }
        function onAccentChanged() { plot.requestPaint() }
    }

    Keys.onPressed: (event) => {
        if (selected < 0 || selected >= points.length)
            return
        const p = points[selected]
        const step = (event.modifiers & Qt.ShiftModifier) ? 5 : 1
        if (event.key === Qt.Key_Left) moved(selected, p.temp - step, p.speed)
        else if (event.key === Qt.Key_Right) moved(selected, p.temp + step, p.speed)
        else if (event.key === Qt.Key_Up) moved(selected, p.temp, p.speed + step)
        else if (event.key === Qt.Key_Down) moved(selected, p.temp, p.speed - step)
        else if (event.key === Qt.Key_Tab && selected < points.length - 1) selected++
        else return
        event.accepted = true
    }

    Canvas {
        id: plot
        anchors.fill: parent
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            const c = getContext("2d")
            c.reset()
            const e = editor
            // the grid and its figures
            c.font = "11px Inter"
            c.lineWidth = 1
            c.strokeStyle = Qt.alpha(Tokens.textPrimary, 0.08)
            c.fillStyle = Tokens.textTertiary
            c.textAlign = "right"
            for (let s = 0; s <= 100; s += 25) {
                const y = Math.round(e.yOf(s)) + 0.5
                c.beginPath(); c.moveTo(e.padLeft, y); c.lineTo(e.padLeft + e.plotW, y); c.stroke()
                c.fillText(s + "%", e.padLeft - 8, y + 4)
            }
            c.textAlign = "center"
            for (let t = e.axisFrom; t <= e.axisTo; t += 10) {
                const x = Math.round(e.xOf(t)) + 0.5
                c.beginPath(); c.moveTo(x, Tokens.s3); c.lineTo(x, Tokens.s3 + e.plotH); c.stroke()
                c.fillText(t + "°", x, Tokens.s3 + e.plotH + 18)
            }
            // where the fan runs flat out whatever the curve says
            const hot = e.xOf(e.tempMax)
            c.fillStyle = Qt.alpha(Tokens.statusCritical, 0.08)
            c.fillRect(hot, Tokens.s3, e.padLeft + e.plotW - hot, e.plotH)

            const pts = e.points
            if (pts.length < 2)
                return
            const ink = e.enabled ? Tokens.accent : Tokens.textDisabled
            c.beginPath()
            c.moveTo(e.padLeft, e.yOf(pts[0].speed))
            for (const p of pts)
                c.lineTo(e.xOf(p.temp), e.yOf(p.speed))
            c.lineTo(e.padLeft + e.plotW, e.yOf(pts[pts.length - 1].speed))
            c.lineTo(e.padLeft + e.plotW, Tokens.s3 + e.plotH)
            c.lineTo(e.padLeft, Tokens.s3 + e.plotH)
            c.closePath()
            c.fillStyle = Qt.alpha(ink, 0.14)
            c.fill()
            c.beginPath()
            c.moveTo(e.padLeft, e.yOf(pts[0].speed))
            for (const p of pts)
                c.lineTo(e.xOf(p.temp), e.yOf(p.speed))
            c.lineTo(e.padLeft + e.plotW, e.yOf(pts[pts.length - 1].speed))
            c.strokeStyle = ink
            c.lineWidth = 2
            c.stroke()

            if (e.now >= 0) {
                const x = Math.round(e.xOf(Math.min(e.now, e.axisTo))) + 0.5
                c.setLineDash([3, 3])
                c.strokeStyle = Tokens.textSecondary
                c.lineWidth = 1
                c.beginPath(); c.moveTo(x, Tokens.s3); c.lineTo(x, Tokens.s3 + e.plotH); c.stroke()
                c.setLineDash([])
            }
        }
    }

    Repeater {
        model: editor.points
        delegate: Rectangle {
            id: dot
            required property var modelData
            required property int index
            readonly property bool chosen: editor.selected === index && editor.activeFocus
            x: editor.xOf(modelData.temp) - width / 2
            y: editor.yOf(modelData.speed) - height / 2
            width: 16
            height: 16
            radius: 8
            color: editor.enabled ? Tokens.accent : Tokens.textDisabled
            border.width: 3
            border.color: Tokens.bgCardSolid
            scale: drag.pressed || drag.containsMouse || chosen ? 1.25 : 1
            Behavior on scale { NumberAnimation { duration: Tokens.durationFaster } }

            QQC2.ToolTip.visible: drag.pressed || drag.containsMouse || chosen
            QQC2.ToolTip.text: i18nc("@info:tooltip a point of the fan curve", "%1 °C, fan at %2%",
                                     modelData.temp, modelData.speed)

            MouseArea {
                id: drag
                anchors.fill: parent
                anchors.margins: -6
                hoverEnabled: true
                enabled: editor.enabled
                cursorShape: Qt.SizeAllCursor
                onPressed: { editor.selected = dot.index; editor.forceActiveFocus() }
                onPositionChanged: (mouse) => {
                    if (!pressed)
                        return
                    const p = mapToItem(editor, mouse.x, mouse.y)
                    editor.moved(dot.index, editor.tOf(p.x), editor.sOf(p.y))
                }
            }
        }
    }
}

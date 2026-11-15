// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The last minute of one reading: a grid, a line and the area under it.
// Drawn once per reading, and not at all while hidden.

import QtQuick
import org.raye.ui

Canvas {
    id: graph

    property var values: []
    property real maximum: 100
    property color color: Tokens.accent
    property bool grid: true
    property bool framed: true

    renderStrategy: Canvas.Cooperative
    onValuesChanged: if (visible) requestPaint()
    onVisibleChanged: if (visible) requestPaint()
    onColorChanged: requestPaint()

    onPaint: {
        const c = getContext("2d")
        c.reset()
        const w = width, h = height
        if (graph.framed) {
            c.strokeStyle = Qt.alpha(graph.color, 0.55)
            c.lineWidth = 1
            c.strokeRect(0.5, 0.5, w - 1, h - 1)
        }
        if (graph.grid) {
            c.strokeStyle = Qt.alpha(graph.color, 0.14)
            c.beginPath()
            for (let i = 1; i < 10; ++i) {
                const y = Math.round(h * i / 10) + 0.5
                c.moveTo(1, y); c.lineTo(w - 1, y)
            }
            for (let i = 1; i < 20; ++i) {
                const x = Math.round(w * i / 20) + 0.5
                c.moveTo(x, 1); c.lineTo(x, h - 1)
            }
            c.stroke()
        }
        const v = graph.values
        if (!v || v.length < 2)
            return
        const max = graph.maximum > 0 ? graph.maximum : 1
        const step = (w - 2) / (v.length - 1)
        c.beginPath()
        c.moveTo(1, h - 1)
        for (let i = 0; i < v.length; ++i)
            c.lineTo(1 + i * step, h - 1 - Math.min(v[i], max) / max * (h - 2))
        c.lineTo(w - 1, h - 1)
        c.closePath()
        c.fillStyle = Qt.alpha(graph.color, 0.16)
        c.fill()
        c.beginPath()
        for (let i = 0; i < v.length; ++i) {
            const x = 1 + i * step
            const y = h - 1 - Math.min(v[i], max) / max * (h - 2)
            if (i === 0) c.moveTo(x, y); else c.lineTo(x, y)
        }
        c.strokeStyle = graph.color
        c.lineWidth = 1.5
        c.stroke()
    }
}

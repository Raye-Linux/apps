// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The line icons of the Control Panel, on a 16 px grid, in the text colour.

import QtQuick
import org.raye.ui

Canvas {
    id: glyph

    // chevron, chevronDown, search, shield, monitor, registry, chip, disk, log, terminal, back, up
    property string kind
    property color color: Tokens.textPrimary
    property color accent: Tokens.accent

    implicitWidth: 16
    implicitHeight: 16
    onColorChanged: requestPaint()
    onKindChanged: requestPaint()

    onPaint: {
        const c = getContext("2d")
        c.reset()
        c.scale(width / 16, height / 16)
        c.strokeStyle = glyph.color
        c.fillStyle = glyph.color
        c.lineWidth = 1.2
        c.lineCap = "round"
        c.lineJoin = "round"
        const line = (x1, y1, x2, y2) => { c.moveTo(x1, y1); c.lineTo(x2, y2) }
        const box = (x, y, w, h) => { c.rect(x, y, w, h) }
        c.beginPath()
        switch (glyph.kind) {
        case "computer":
            box(1.5, 2.5, 13, 8.5)
            line(8, 11, 8, 13.5); line(5, 13.5, 11, 13.5)
            break
        case "folder":
        case "folderOpen":
            c.moveTo(1.5, 4); c.lineTo(1.5, 13); c.lineTo(14.5, 13); c.lineTo(14.5, 5.5)
            c.lineTo(7.5, 5.5); c.lineTo(6, 3.5); c.lineTo(2, 3.5); c.closePath()
            if (glyph.kind === "folderOpen") {
                c.stroke(); c.beginPath()
                c.globalAlpha = 0.35; c.fillStyle = glyph.accent
                c.fillRect(2, 7, 12, 5.5); c.globalAlpha = 1
            }
            break
        case "file":
            c.moveTo(3.5, 1.5); c.lineTo(9.5, 1.5); c.lineTo(12.5, 4.5); c.lineTo(12.5, 14.5); c.lineTo(3.5, 14.5); c.closePath()
            line(9.5, 1.5, 9.5, 4.5); line(9.5, 4.5, 12.5, 4.5)
            line(5.5, 8, 10.5, 8); line(5.5, 10.5, 10.5, 10.5)
            break
        case "lock":
            box(3.5, 7, 9, 7)
            c.moveTo(5.5, 7); c.lineTo(5.5, 5); c.arc(8, 5, 2.5, Math.PI, 0); c.lineTo(10.5, 7)
            break
        case "text":
            // "ab" in two strokes, the way the type column of a registry reads
            c.moveTo(6, 10.5); c.arc(4, 10.5, 2, 0, Math.PI * 2)
            line(6, 8.5, 6, 12.5)
            line(9, 4, 9, 12.5); c.moveTo(13.5, 10.5); c.arc(11.25, 10.5, 2.25, 0, Math.PI * 2)
            break
        case "number":
            c.moveTo(4.5, 4); c.arc(3.75, 8, 2.25, -Math.PI / 2, Math.PI * 1.5)
            line(9, 5, 10, 4); line(10, 4, 10, 12)
            line(13, 5, 14, 4); line(14, 4, 14, 12)
            break
        case "yesno":
            c.arc(8, 8, 6, 0, Math.PI * 2)
            c.stroke(); c.beginPath()
            line(5.2, 8.2, 7.2, 10.2); line(7.2, 10.2, 11, 6)
            break
        case "list":
            for (let y of [4, 8, 12]) { line(2.5, y, 3.5, y); line(6, y, 13.5, y) }
            break
        case "chevron":
            line(6, 4, 10, 8); line(10, 8, 6, 12)
            break
        case "chevronDown":
            line(4, 6, 8, 10); line(8, 10, 12, 6)
            break
        case "monitor":
            c.moveTo(1.5, 9); c.lineTo(4.5, 9); c.lineTo(6.5, 4); c.lineTo(9.5, 13); c.lineTo(11.5, 8); c.lineTo(14.5, 8)
            break
        case "registry":
            box(2, 2, 5, 5); box(9, 2, 5, 5); box(2, 9, 5, 5); box(9, 9, 5, 5)
            break
        case "chip":
            box(4, 4, 8, 8)
            for (let p of [6, 8, 10]) { line(p, 1.5, p, 4); line(p, 12, p, 14.5); line(1.5, p, 4, p); line(12, p, 14.5, p) }
            break
        case "disk":
            box(1.5, 5, 13, 6)
            c.moveTo(12.5, 8); c.arc(12, 8, 0.5, 0, Math.PI * 2)
            break
        case "log":
            c.moveTo(3.5, 1.5); c.lineTo(12.5, 1.5); c.lineTo(12.5, 14.5); c.lineTo(3.5, 14.5); c.closePath()
            for (let y of [5, 8, 11]) line(5.5, y, 10.5, y)
            break
        case "terminal":
            box(1.5, 2.5, 13, 11)
            line(4, 6, 6.5, 8); line(6.5, 8, 4, 10); line(8, 10.5, 11.5, 10.5)
            break
        case "back":
            line(13, 8, 3, 8); line(3, 8, 7, 4); line(3, 8, 7, 12)
            break
        case "up":
            line(8, 13, 8, 3); line(8, 3, 4, 7); line(8, 3, 12, 7)
            break
        case "search":
            c.arc(7, 7, 4.5, 0, Math.PI * 2)
            line(10.3, 10.3, 14, 14)
            break
        case "star":
            for (let i = 0; i <= 10; ++i) {
                const a = -Math.PI / 2 + i * Math.PI / 5
                const r = i % 2 === 0 ? 6.5 : 2.8
                if (i === 0) c.moveTo(8 + Math.cos(a) * r, 8.5 + Math.sin(a) * r)
                else c.lineTo(8 + Math.cos(a) * r, 8.5 + Math.sin(a) * r)
            }
            break
        case "shield":
            c.moveTo(8, 1.5); c.lineTo(13.5, 3.5); c.lineTo(13.5, 8)
            c.bezierCurveTo(13.5, 11.5, 11, 13.5, 8, 14.5)
            c.bezierCurveTo(5, 13.5, 2.5, 11.5, 2.5, 8); c.lineTo(2.5, 3.5); c.closePath()
            break
        }
        c.stroke()
    }
}

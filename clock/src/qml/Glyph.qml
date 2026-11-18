// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The clock's line icons, drawn on a 16 px grid with one stroke.

import QtQuick
import org.raye.ui

Canvas {
    id: glyph

    // menu, add, edit, delete, play, pause, reset, flag, bell, snooze, check,
    // chevronDown, close, timer, alarm, stopwatch, globe, sun, moon, search, sound
    property string kind
    property color color: Tokens.textPrimary
    property real stroke: 1.25

    implicitWidth: 16
    implicitHeight: 16
    onColorChanged: requestPaint()
    onKindChanged: requestPaint()
    onWidthChanged: requestPaint()

    onPaint: {
        const c = getContext("2d")
        c.reset()
        c.scale(width / 16, height / 16)
        c.strokeStyle = glyph.color
        c.fillStyle = glyph.color
        c.lineWidth = glyph.stroke
        c.lineCap = "round"
        c.lineJoin = "round"
        const line = (x1, y1, x2, y2) => { c.moveTo(x1, y1); c.lineTo(x2, y2) }
        const circle = (x, y, r) => { c.moveTo(x + r, y); c.arc(x, y, r, 0, Math.PI * 2) }
        c.beginPath()
        switch (glyph.kind) {
        case "menu":
            line(2.5, 4.5, 13.5, 4.5); line(2.5, 8, 13.5, 8); line(2.5, 11.5, 13.5, 11.5)
            break
        case "add":
            line(8, 2.5, 8, 13.5); line(2.5, 8, 13.5, 8)
            break
        case "close":
            line(4, 4, 12, 12); line(12, 4, 4, 12)
            break
        case "check":
            line(3.5, 8.5, 6.5, 11.5); line(6.5, 11.5, 12.5, 4.5)
            break
        case "chevronDown":
            line(4, 6, 8, 10); line(8, 10, 12, 6)
            break
        case "edit":
            c.moveTo(3, 13); c.lineTo(3.6, 10.4); c.lineTo(10.6, 3.4)
            c.quadraticCurveTo(11.6, 2.4, 12.6, 3.4); c.quadraticCurveTo(13.6, 4.4, 12.6, 5.4)
            c.lineTo(5.6, 12.4); c.closePath()
            line(9.6, 4.4, 11.6, 6.4)
            break
        case "delete":
            line(2.5, 4, 13.5, 4)
            c.moveTo(6, 4); c.lineTo(6.4, 2.5); c.lineTo(9.6, 2.5); c.lineTo(10, 4)
            c.moveTo(4, 4); c.lineTo(4.8, 13.5); c.lineTo(11.2, 13.5); c.lineTo(12, 4)
            line(6.7, 6.5, 6.9, 11); line(9.3, 6.5, 9.1, 11)
            break
        case "play":
            c.moveTo(5, 3); c.lineTo(13, 8); c.lineTo(5, 13); c.closePath()
            c.fill()
            break
        case "pause":
            c.rect(4, 3, 2.6, 10); c.rect(9.4, 3, 2.6, 10)
            c.fill()
            break
        case "reset":
            c.moveTo(3, 8); c.arc(8, 8, 5, Math.PI, Math.PI * 2.6, false)
            line(3, 8, 3, 4.5); line(3, 8, 6.3, 7.6)
            break
        case "flag":
            line(4, 14, 4, 2.5)
            c.moveTo(4, 3); c.lineTo(12.5, 3); c.lineTo(10.5, 6); c.lineTo(12.5, 9); c.lineTo(4, 9)
            break
        case "bell":
            c.moveTo(4, 11.5); c.lineTo(4, 7.5); c.bezierCurveTo(4, 4.8, 5.8, 3, 8, 3)
            c.bezierCurveTo(10.2, 3, 12, 4.8, 12, 7.5); c.lineTo(12, 11.5)
            line(2.8, 11.5, 13.2, 11.5)
            c.moveTo(6.6, 13.5); c.quadraticCurveTo(8, 14.6, 9.4, 13.5)
            break
        case "snooze":
            c.moveTo(4, 11.5); c.lineTo(4, 7.5); c.bezierCurveTo(4, 4.8, 5.8, 3, 8, 3)
            c.bezierCurveTo(10.2, 3, 12, 4.8, 12, 7.5); c.lineTo(12, 11.5)
            line(2.8, 11.5, 13.2, 11.5)
            line(6.5, 6, 9.5, 6); line(9.5, 6, 6.5, 9); line(6.5, 9, 9.5, 9)
            break
        case "timer":
            c.moveTo(4, 2.5); c.lineTo(12, 2.5); c.moveTo(4, 13.5); c.lineTo(12, 13.5)
            c.moveTo(5, 2.5); c.bezierCurveTo(5, 6, 8, 6.5, 8, 8); c.bezierCurveTo(8, 9.5, 5, 10, 5, 13.5)
            c.moveTo(11, 2.5); c.bezierCurveTo(11, 6, 8, 6.5, 8, 8); c.bezierCurveTo(8, 9.5, 11, 10, 11, 13.5)
            break
        case "alarm":
            circle(8, 9, 5)
            line(8, 6.5, 8, 9); line(8, 9, 9.8, 10.2)
            line(2.5, 4, 4.5, 2.3); line(13.5, 4, 11.5, 2.3)
            line(4.6, 13.2, 3.6, 14.4); line(11.4, 13.2, 12.4, 14.4)
            break
        case "stopwatch":
            circle(8, 9.2, 5.3)
            line(8, 9.2, 8, 6.2); line(6.5, 1.8, 9.5, 1.8); line(8, 1.8, 8, 3.9)
            line(12, 4.6, 13, 3.6)
            break
        case "globe":
            circle(8, 8, 5.8)
            c.moveTo(8, 2.2); c.bezierCurveTo(5.2, 5, 5.2, 11, 8, 13.8)
            c.moveTo(8, 2.2); c.bezierCurveTo(10.8, 5, 10.8, 11, 8, 13.8)
            line(2.4, 8, 13.6, 8); line(3.3, 5, 12.7, 5); line(3.3, 11, 12.7, 11)
            break
        case "sun":
            circle(8, 8, 2.8)
            for (let i = 0; i < 8; ++i) {
                const a = i * Math.PI / 4
                line(8 + Math.cos(a) * 4.7, 8 + Math.sin(a) * 4.7, 8 + Math.cos(a) * 6.2, 8 + Math.sin(a) * 6.2)
            }
            break
        case "moon": {
            // a crescent: the outer circle the long way round, the bite back in
            const ox = 7.4, oy = 8.6, R = 5.6, ix = 10.6, iy = 5.4, r = 4.6
            const dx = ix - ox, dy = iy - oy, d = Math.hypot(dx, dy)
            const a = (R * R - r * r + d * d) / (2 * d), h = Math.sqrt(R * R - a * a)
            const mx = ox + a * dx / d, my = oy + a * dy / d
            const p1 = [mx + h * dy / d, my - h * dx / d], p2 = [mx - h * dy / d, my + h * dx / d]
            const ang = (cx, cy, p) => Math.atan2(p[1] - cy, p[0] - cx)
            c.moveTo(p1[0], p1[1])
            c.arc(ox, oy, R, ang(ox, oy, p1), ang(ox, oy, p2), true)
            c.arc(ix, iy, r, ang(ix, iy, p2), ang(ix, iy, p1), false)
            c.closePath()
            break
        }
        case "search":
            circle(7, 7, 4.3)
            line(10.2, 10.2, 13.5, 13.5)
            break
        case "sound":
            c.moveTo(2.5, 6); c.lineTo(5, 6); c.lineTo(8.5, 3); c.lineTo(8.5, 13); c.lineTo(5, 10)
            c.lineTo(2.5, 10); c.closePath()
            c.moveTo(10.5, 5.5); c.quadraticCurveTo(12, 8, 10.5, 10.5)
            c.moveTo(12, 3.8); c.quadraticCurveTo(14.6, 8, 12, 12.2)
            break
        }
        c.stroke()
    }
}

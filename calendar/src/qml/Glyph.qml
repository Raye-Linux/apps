// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The calendar's line icons, drawn on a 16 px grid with one stroke.

import QtQuick
import org.raye.ui

Canvas {
    id: glyph

    // add, back, forward, more, bell, repeat, today, chevronDown, check, pin
    property string kind
    property color color: Tokens.textPrimary

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
        c.lineWidth = 1.25
        c.lineCap = "round"
        c.lineJoin = "round"
        const line = (x1, y1, x2, y2) => { c.moveTo(x1, y1); c.lineTo(x2, y2) }
        c.beginPath()
        switch (glyph.kind) {
        case "menu":
            line(2.5, 4.5, 13.5, 4.5); line(2.5, 8, 13.5, 8); line(2.5, 11.5, 13.5, 11.5)
            break
        case "add":
            line(8, 2.5, 8, 13.5); line(2.5, 8, 13.5, 8)
            break
        case "back":
            line(10, 3, 5, 8); line(5, 8, 10, 13)
            break
        case "forward":
            line(6, 3, 11, 8); line(11, 8, 6, 13)
            break
        case "chevronDown":
            line(4, 6, 8, 10); line(8, 10, 12, 6)
            break
        case "more":
            c.arc(3.5, 8, 0.9, 0, Math.PI * 2); c.moveTo(8.9, 8)
            c.arc(8, 8, 0.9, 0, Math.PI * 2); c.moveTo(13.4, 8)
            c.arc(12.5, 8, 0.9, 0, Math.PI * 2)
            c.fill()
            break
        case "bell":
            c.moveTo(4, 11.5); c.lineTo(4, 7.5); c.bezierCurveTo(4, 4.8, 5.8, 3, 8, 3)
            c.bezierCurveTo(10.2, 3, 12, 4.8, 12, 7.5); c.lineTo(12, 11.5)
            line(2.8, 11.5, 13.2, 11.5)
            c.moveTo(6.6, 13.5); c.quadraticCurveTo(8, 14.6, 9.4, 13.5)
            break
        case "repeat":
            c.moveTo(3, 7.5); c.bezierCurveTo(3, 5, 4.8, 4, 7, 4); c.lineTo(12.5, 4)
            line(10.5, 2, 12.5, 4); line(12.5, 4, 10.5, 6)
            c.moveTo(13, 8.5); c.bezierCurveTo(13, 11, 11.2, 12, 9, 12); c.lineTo(3.5, 12)
            line(5.5, 10, 3.5, 12); line(3.5, 12, 5.5, 14)
            break
        case "today":
            c.rect(2.5, 3.5, 11, 10)
            line(2.5, 6.5, 13.5, 6.5); line(5.5, 2, 5.5, 4.5); line(10.5, 2, 10.5, 4.5)
            c.stroke(); c.beginPath()
            c.arc(8, 10, 1.6, 0, Math.PI * 2)
            c.fill()
            break
        case "check":
            line(3.5, 8.5, 6.5, 11.5); line(6.5, 11.5, 12.5, 4.5)
            break
        case "pin":
            c.moveTo(8, 14); c.bezierCurveTo(5, 10.5, 4, 8.8, 4, 6.8)
            c.bezierCurveTo(4, 4.4, 5.8, 2.5, 8, 2.5); c.bezierCurveTo(10.2, 2.5, 12, 4.4, 12, 6.8)
            c.bezierCurveTo(12, 8.8, 11, 10.5, 8, 14)
            c.moveTo(9.6, 6.8); c.arc(8, 6.8, 1.6, 0, Math.PI * 2)
            break
        }
        c.stroke()
    }
}

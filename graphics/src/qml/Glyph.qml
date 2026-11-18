// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The line icons of the panel, drawn on a 16 px grid so they share one
// stroke and follow the text colour.

import QtQuick
import org.raye.ui

Canvas {
    id: glyph

    // menu, overview, tune, fan, card, chevronDown, open, restore
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
        c.lineWidth = 1.25
        c.lineCap = "round"
        c.lineJoin = "round"
        const line = (x1, y1, x2, y2) => { c.moveTo(x1, y1); c.lineTo(x2, y2) }
        c.beginPath()
        switch (glyph.kind) {
        case "menu":
            line(2, 4, 14, 4); line(2, 8, 14, 8); line(2, 12, 14, 12)
            break
        case "overview":
            c.moveTo(1.5, 9); c.lineTo(4.5, 9); c.lineTo(6.5, 4); c.lineTo(9.5, 13); c.lineTo(11.5, 8); c.lineTo(14.5, 8)
            break
        case "tune":
            line(2, 4, 14, 4); line(2, 8, 14, 8); line(2, 12, 14, 12)
            c.moveTo(6.5, 4); c.arc(5, 4, 1.5, 0, Math.PI * 2)
            c.moveTo(12.5, 8); c.arc(11, 8, 1.5, 0, Math.PI * 2)
            c.moveTo(8.5, 12); c.arc(7, 12, 1.5, 0, Math.PI * 2)
            break
        case "fan":
            c.moveTo(9.3, 8); c.arc(8, 8, 1.3, 0, Math.PI * 2)
            for (let i = 0; i < 3; ++i) {
                const a = i * Math.PI * 2 / 3 - Math.PI / 2
                const tip = (r, d) => [8 + Math.cos(a + d) * r, 8 + Math.sin(a + d) * r]
                const [x0, y0] = tip(1.6, 0)
                const [x1, y1] = tip(6.5, -0.15)
                const [x2, y2] = tip(5.5, 0.75)
                c.moveTo(x0, y0)
                c.quadraticCurveTo(...tip(5, -0.45), x1, y1)
                c.quadraticCurveTo(...tip(7, 0.35), x2, y2)
                c.quadraticCurveTo(...tip(3, 0.6), x0, y0)
            }
            break
        case "card":
            c.rect(1.5, 4, 13, 8)
            c.moveTo(7.5, 8); c.arc(5.5, 8, 2, 0, Math.PI * 2)
            line(9.5, 6.5, 12.5, 6.5); line(9.5, 9.5, 12.5, 9.5)
            line(3.5, 12, 3.5, 14); line(6, 12, 6, 14); line(8.5, 12, 8.5, 14)
            break
        case "chevronDown":
            line(4, 6, 8, 10); line(8, 10, 12, 6)
            break
        case "open":
            c.moveTo(12, 9); c.lineTo(12, 13); c.lineTo(3, 13); c.lineTo(3, 4); c.lineTo(7, 4)
            line(9.5, 2.5, 13.5, 2.5); line(13.5, 2.5, 13.5, 6.5); line(13.5, 2.5, 7.5, 8.5)
            break
        case "restore":
            c.arc(8, 8, 5.5, Math.PI * 1.1, Math.PI * 2.8)
            line(2.6, 3.5, 2.6, 6.8); line(2.6, 6.8, 5.8, 6.8)
            break
        }
        c.stroke()
    }
}

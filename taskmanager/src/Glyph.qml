// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The line icons of the navigation, drawn here on a 16 px grid so they
// share one stroke and follow the text colour.

import QtQuick
import org.raye.ui

Canvas {
    id: glyph

    // menu, processes, performance, startup, users, details, services, settings, run, end, leaf
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
            line(2, 4, 14, 4); line(2, 8, 14, 8); line(2, 12, 14, 12)
            break
        case "processes":
            c.rect(2, 2, 5, 5); c.rect(9, 2, 5, 5); c.rect(2, 9, 5, 5); c.rect(9, 9, 5, 5)
            break
        case "performance":
            c.moveTo(1.5, 9); c.lineTo(4.5, 9); c.lineTo(6.5, 4); c.lineTo(9.5, 13); c.lineTo(11.5, 8); c.lineTo(14.5, 8)
            break
        case "startup":
            c.arc(8, 9, 5.5, Math.PI * 0.8, Math.PI * 2.2)
            line(8, 9, 11, 6)
            break
        case "users":
            c.arc(6, 5.5, 2.5, 0, Math.PI * 2)
            c.moveTo(1.5, 14); c.quadraticCurveTo(6, 7.5, 10.5, 14)
            c.moveTo(11.8, 3.2); c.arc(11, 5.5, 2.2, -Math.PI / 3, Math.PI * 0.6)
            c.moveTo(12, 9.5); c.quadraticCurveTo(14, 10.5, 14.5, 13)
            break
        case "details":
            for (let y of [4, 8, 12]) {
                c.moveTo(3.3, y); c.arc(2.6, y, 0.7, 0, Math.PI * 2)
                line(5.5, y, 14, y)
            }
            break
        case "services":
            c.arc(8, 8, 2.2, 0, Math.PI * 2)
            for (let i = 0; i < 8; ++i) {
                const a = i * Math.PI / 4
                line(8 + Math.cos(a) * 4.2, 8 + Math.sin(a) * 4.2, 8 + Math.cos(a) * 6, 8 + Math.sin(a) * 6)
            }
            c.moveTo(12.2, 8); c.arc(8, 8, 4.2, 0, Math.PI * 2)
            break
        case "settings":
            c.arc(8, 8, 2, 0, Math.PI * 2)
            c.moveTo(13.5, 8)
            for (let i = 0; i <= 12; ++i) {
                const a = i * Math.PI / 6
                const r = i % 2 === 0 ? 5.5 : 4
                c.lineTo(8 + Math.cos(a) * r, 8 + Math.sin(a) * r)
            }
            break
        case "run":
            c.rect(2, 3, 12, 10)
            line(4.5, 6, 7, 8); line(7, 8, 4.5, 10); line(8.5, 10.5, 11.5, 10.5)
            break
        case "end":
            c.arc(8, 8, 6, 0, Math.PI * 2)
            line(5.5, 5.5, 10.5, 10.5); line(10.5, 5.5, 5.5, 10.5)
            break
        case "leaf":
            c.moveTo(3, 13); c.bezierCurveTo(2, 6, 7, 2, 14, 2); c.bezierCurveTo(14, 9, 10, 14, 3, 13)
            line(3, 13, 9, 7)
            break
        case "chevron":
            line(6, 4, 10, 8); line(10, 8, 6, 12)
            break
        case "chevronDown":
            line(4, 6, 8, 10); line(8, 10, 12, 6)
            break
        case "sortUp":
            line(5, 9, 8, 6); line(8, 6, 11, 9)
            break
        case "sortDown":
            line(5, 7, 8, 10); line(8, 10, 11, 7)
            break
        case "search":
            c.arc(7, 7, 4.5, 0, Math.PI * 2)
            line(10.3, 10.3, 14, 14)
            break
        }
        c.stroke()
    }
}

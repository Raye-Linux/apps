// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The ring around a timer: a quiet track, and the accent arc of what is
// left, running down counterclockwise from the top. Round ends.

import QtQuick
import QtQuick.Shapes
import org.raye.ui

Item {
    id: ring

    // 1 full, 0 empty
    property real value: 1
    property real thickness: 6
    property color color: Tokens.accent

    readonly property real r: (Math.min(width, height) - thickness) / 2

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            fillColor: "transparent"
            strokeColor: Tokens.strokeDivider
            strokeWidth: ring.thickness
            PathAngleArc {
                centerX: ring.width / 2
                centerY: ring.height / 2
                radiusX: ring.r
                radiusY: ring.r
                startAngle: 0
                sweepAngle: 360
            }
        }
        ShapePath {
            fillColor: "transparent"
            strokeColor: ring.value > 0.0005 ? ring.color : "transparent"
            strokeWidth: ring.thickness
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                centerX: ring.width / 2
                centerY: ring.height / 2
                radiusX: ring.r
                radiusY: ring.r
                startAngle: -90
                sweepAngle: -360 * Math.max(0, Math.min(1, ring.value))
            }
        }
    }
}

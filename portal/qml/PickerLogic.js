.pragma library
// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

// What the share picker decides without drawing anything: which tabs it
// has, what each row of the screens model is, the numbers in the labels.
// Kept apart so the tests can check it without a window.

// source types and cursor modes, numbered as in the ScreenCast portal
var SourceMonitor = 1
var SourceWindow = 2
var SourceVirtual = 4
var CursorHidden = 1

// The synthetic rows of the backend's screens model come after the real
// screens, always in this order (outputsmodel.cpp): "All screens" when
// screens were asked for and there are several, a virtual screen, then
// "Share region" when screens were asked for. The model names them only
// in words of the desktop's catalog, so the picker tells them apart by
// their place. When the count does not match (a newer backend), they
// are all "other" and stay in the screens tab as plain cards.
function syntheticKinds(sourceTypes, realScreens, syntheticCount) {
    var kinds = []
    if (sourceTypes & SourceMonitor) {
        if (realScreens > 1)
            kinds.push("workspace")
        kinds.push("virtual")
        kinds.push("region")
    } else if (sourceTypes & SourceVirtual) {
        kinds.push("virtual")
    }
    if (kinds.length !== syntheticCount) {
        kinds = []
        for (var i = 0; i < syntheticCount; ++i)
            kinds.push("other")
    }
    return kinds
}

// The kind of row `row` of the screens model: "screen" for a real one,
// else one of syntheticKinds()
function outputKind(row, rowCount, syntheticCount, sourceTypes) {
    var real = rowCount - syntheticCount
    if (row < real)
        return "screen"
    var kinds = syntheticKinds(sourceTypes, real, syntheticCount)
    return kinds[row - real] || "other"
}

// The tab a row of the screens model is shown in
function tabOfKind(kind) {
    return kind === "region" ? "area" : "screen"
}

// The tabs, in order. hasOutputs/hasWindows: whether the backend gave
// that model; the area tab only when the region row exists.
function tabs(sourceTypes, hasOutputs, hasWindows, rowCount, syntheticCount) {
    var list = []
    if (hasOutputs && rowCount > 0) {
        var real = rowCount - syntheticCount
        var kinds = syntheticKinds(sourceTypes, real, syntheticCount)
        var screenTab = real > 0
        var areaTab = false
        for (var i = 0; i < kinds.length; ++i) {
            if (kinds[i] === "region")
                areaTab = true
            else
                screenTab = true
        }
        if (screenTab)
            list.push("screen")
        if (hasWindows)
            list.push("window")
        if (areaTab)
            list.push("area")
    } else if (hasWindows) {
        list.push("window")
    }
    return list
}

// What the first line says the app wants to share
function requestKind(sourceTypes) {
    if (sourceTypes === SourceWindow)
        return "window"
    if (sourceTypes === SourceVirtual)
        return "virtual"
    return "screen"
}

// Real screens are "Screen 1", "Screen 2"... in the model's order
function screenNumber(row) {
    return row + 1
}

// The size in device pixels of a screen: its logical geometry times its
// scale, what the user set in Display settings
function pixelSize(width, height, scale) {
    var s = scale > 0 ? scale : 1
    return { width: Math.round(width * s), height: Math.round(height * s) }
}

// The row to choose before the user does anything, so Enter shares it
// right away: the only screen when the app asked for a screen and there
// is just one, or the virtual screen when that is all it asked for.
// -1 when the user has to choose.
function preselectedRow(sourceTypes, rowCount, syntheticCount) {
    if (sourceTypes === SourceMonitor && rowCount - syntheticCount === 1)
        return 0
    if (sourceTypes === SourceVirtual && rowCount === 1)
        return 0
    return -1
}

// "Share the pointer" is offered when the app lets the pointer be sent
function offersPointer(cursorMode) {
    return cursorMode > CursorHidden
}

// How many cards fit on a line
function columns(availableWidth, cellWidth) {
    if (cellWidth <= 0)
        return 1
    return Math.max(1, Math.floor(availableWidth / cellWidth))
}

// Text that goes into a rich text label as it is
function escapeHtml(text) {
    return String(text === undefined || text === null ? "" : text)
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#39;")
}

// The title of a window, or "" when it has none worth showing
function windowTitle(title) {
    var t = String(title === undefined || title === null ? "" : title).trim()
    return t
}

// The next tab with arrow keys: wraps around, like Windows' segmented tabs
function nextTab(list, current, step) {
    if (list.length === 0)
        return ""
    var i = list.indexOf(current)
    if (i < 0)
        return list[0]
    return list[(i + step + list.length) % list.length]
}

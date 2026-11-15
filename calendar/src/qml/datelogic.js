// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Day arithmetic of the views, in local time. Qt's firstDayOfWeek counts
// Monday as 1 and Sunday as 7; JavaScript's getDay() says 0 for Sunday.
.pragma library

function dayStart(d) {
    return new Date(d.getFullYear(), d.getMonth(), d.getDate())
}

function addDays(d, n) {
    return new Date(d.getFullYear(), d.getMonth(), d.getDate() + n, d.getHours(), d.getMinutes())
}

function sameDay(a, b) {
    return a.getFullYear() === b.getFullYear() && a.getMonth() === b.getMonth() && a.getDate() === b.getDate()
}

function weekStart(d, firstDay) {
    const js = firstDay % 7
    const back = (d.getDay() - js + 7) % 7
    return addDays(dayStart(d), -back)
}

// the 42 days of a month page, starting on the locale's first day of the week
function monthCells(year, month, firstDay) {
    const start = weekStart(new Date(year, month, 1), firstDay)
    const cells = []
    for (let i = 0; i < 42; ++i)
        cells.push(addDays(start, i))
    return cells
}

function isoDay(d) {
    const m = d.getMonth() + 1, day = d.getDate()
    return d.getFullYear() + "-" + (m < 10 ? "0" : "") + m + "-" + (day < 10 ? "0" : "") + day
}

function fromIsoDay(s) {
    const p = String(s).split("-")
    if (p.length !== 3)
        return null
    const d = new Date(Number(p[0]), Number(p[1]) - 1, Number(p[2]))
    return isNaN(d.getTime()) ? null : d
}

// occurrences that touch the day [d, d+1)
function onDay(list, d) {
    const s = dayStart(d), e = addDays(s, 1)
    return list.filter(o => o.start < e && (o.end > s || (o.end.getTime() === o.start.getTime() && o.start >= s)))
}

// Side-by-side places for the timed events of one day: each gets a lane
// and the number of lanes of the group it overlaps with.
function lanes(events) {
    const sorted = events.slice().sort((a, b) => a.start - b.start || b.end - a.end)
    const out = []
    let group = [], groupEnd = 0, laneEnds = []
    function close() {
        for (const g of group)
            g.count = laneEnds.length
        group = []
        laneEnds = []
    }
    for (const ev of sorted) {
        const start = ev.start.getTime(), end = Math.max(ev.end.getTime(), start + 15 * 60000)
        if (group.length && start >= groupEnd)
            close()
        let lane = laneEnds.findIndex(t => t <= start)
        if (lane < 0) {
            lane = laneEnds.length
            laneEnds.push(end)
        } else {
            laneEnds[lane] = end
        }
        const placed = { ev: ev, lane: lane, count: 1 }
        group.push(placed)
        out.push(placed)
        groupEnd = Math.max(groupEnd, end)
    }
    close()
    return out
}

// minutes since midnight of d, cut to the day `day`
function minutesIn(d, day) {
    const s = dayStart(day)
    const m = (d - s) / 60000
    return Math.max(0, Math.min(24 * 60, m))
}

// "9:30 AM" or "09:30": the locale's short time, never with seconds
function timeText(d) {
    const f = Qt.locale().timeFormat(1).replace(/[:.]ss/, "").replace(/\s*t$/, "")
    return Qt.formatTime(d, f)
}

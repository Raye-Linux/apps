// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The search box of the Control Panel. tests/test_search.cpp runs it.
.pragma library

// "École" is found by "ecole"
function fold(text) {
    return String(text).toLowerCase().normalize("NFD").replace(/[\u0300-\u036f]/g, "")
}

// every word typed has to be in the text
function hits(query, text) {
    const hay = fold(text)
    return fold(query).split(/\s+/).filter(w => w !== "").every(w => hay.indexOf(w) >= 0)
}

function words(item) {
    return [item.name, item.summary, item.keywords || ""].concat((item.tasks || []).map(t => t.text)).join(" ")
}

// entries: [{ item, tasks }] with the tasks this PC can open. Each item
// that matches comes back with the tasks that match, or its first three
// when only its name or keywords did.
function results(query, entries) {
    if (fold(query).trim() === "")
        return []
    const found = []
    for (const e of entries) {
        const tasks = e.tasks.filter(t => hits(query, t.text))
        if (tasks.length > 0)
            found.push({ item: e.item, tasks: tasks })
        else if (hits(query, words(e.item)))
            found.push({ item: e.item, tasks: e.tasks.slice(0, 3) })
    }
    return found
}

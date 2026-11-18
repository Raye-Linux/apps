// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The world clocks' arithmetic: how far a place is from here, which day it
// is there, whether the sun is up, and the search through the places.

#pragma once

#include <QDateTime>
#include <QStringList>
#include <QTimeZone>

namespace WorldTime
{
struct Gap
{
    // the place's offset minus ours, in minutes (+300: five hours ahead)
    int minutes = 0;
    // its date minus ours: -1 yesterday, 0 today, 1 tomorrow
    int days = 0;
};
Gap compare(const QTimeZone &place, const QTimeZone &here, const QDateTime &now);

// a rough "daytime" for the card's colour: 6:00 to 18:59 local time there
bool daytime(const QTimeZone &place, const QDateTime &now);

// lower case, accents dropped, for a search that finds "Montréal" from "montreal"
QString fold(const QString &text);
// every word of the query starts a word of one of the fields
bool matches(const QString &query, const QStringList &fields);

// "Europe/Paris" -> "Paris", "America/Argentina/Buenos_Aires" -> "Buenos Aires"
QString cityOf(const QByteArray &zoneId);
}

// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The places the world clocks offer, with their names in the user's
// language: the big cities by hand (several can share a zone: Mumbai and
// New Delhi), and every other zone of the system under the city name of
// its own id.

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

struct City
{
    // what the settings keep: "zone|English name", or the zone alone
    QString key;
    QByteArray zone;
    QString name;
    QString country;
    // a city named by hand, shown first in a search
    bool known = false;
};

namespace Cities
{
QList<City> all();
// the city of a key, or a city with an empty zone when the key is unknown
City of(const QString &key);
}

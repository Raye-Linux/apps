// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cities.h"
#include "worldtime.h"

#include <KCountry>
#include <KLazyLocalizedString>
#include <KLocalizedString>
#include <KTimeZone>

#include <QSet>
#include <QTimeZone>

namespace
{
struct Known
{
    const char *zone;
    KLazyLocalizedString name;
};

// clang-format off
const Known kKnown[] = {
    {"Pacific/Honolulu", kli18nc("@item city", "Honolulu")},
    {"America/Anchorage", kli18nc("@item city", "Anchorage")},
    {"America/Vancouver", kli18nc("@item city", "Vancouver")},
    {"America/Los_Angeles", kli18nc("@item city", "Los Angeles")},
    {"America/Los_Angeles", kli18nc("@item city", "San Francisco")},
    {"America/Los_Angeles", kli18nc("@item city", "Seattle")},
    {"America/Phoenix", kli18nc("@item city", "Phoenix")},
    {"America/Denver", kli18nc("@item city", "Denver")},
    {"America/Edmonton", kli18nc("@item city", "Edmonton")},
    {"America/Edmonton", kli18nc("@item city", "Calgary")},
    {"America/Regina", kli18nc("@item city", "Regina")},
    {"America/Winnipeg", kli18nc("@item city", "Winnipeg")},
    {"America/Chicago", kli18nc("@item city", "Chicago")},
    {"America/Chicago", kli18nc("@item city", "Dallas")},
    {"America/Chicago", kli18nc("@item city", "Houston")},
    {"America/Mexico_City", kli18nc("@item city", "Mexico City")},
    {"America/Toronto", kli18nc("@item city", "Toronto")},
    {"America/Toronto", kli18nc("@item city", "Ottawa")},
    {"America/Toronto", kli18nc("@item city", "Montreal")},
    {"America/Toronto", kli18nc("@item city", "Quebec City")},
    {"America/New_York", kli18nc("@item city", "New York")},
    {"America/New_York", kli18nc("@item city", "Washington")},
    {"America/New_York", kli18nc("@item city", "Boston")},
    {"America/New_York", kli18nc("@item city", "Miami")},
    {"America/New_York", kli18nc("@item city", "Atlanta")},
    {"America/Halifax", kli18nc("@item city", "Halifax")},
    {"America/Moncton", kli18nc("@item city", "Moncton")},
    {"America/St_Johns", kli18nc("@item city", "St. John's")},
    {"America/Havana", kli18nc("@item city", "Havana")},
    {"America/Port-au-Prince", kli18nc("@item city", "Port-au-Prince")},
    {"America/Puerto_Rico", kli18nc("@item city", "San Juan")},
    {"America/Martinique", kli18nc("@item city", "Fort-de-France")},
    {"America/Guadeloupe", kli18nc("@item city", "Pointe-à-Pitre")},
    {"America/Bogota", kli18nc("@item city", "Bogotá")},
    {"America/Lima", kli18nc("@item city", "Lima")},
    {"America/Caracas", kli18nc("@item city", "Caracas")},
    {"America/Cayenne", kli18nc("@item city", "Cayenne")},
    {"America/Santiago", kli18nc("@item city", "Santiago")},
    {"America/Argentina/Buenos_Aires", kli18nc("@item city", "Buenos Aires")},
    {"America/Sao_Paulo", kli18nc("@item city", "São Paulo")},
    {"America/Sao_Paulo", kli18nc("@item city", "Rio de Janeiro")},
    {"Atlantic/Reykjavik", kli18nc("@item city", "Reykjavik")},
    {"Europe/Dublin", kli18nc("@item city", "Dublin")},
    {"Europe/London", kli18nc("@item city", "London")},
    {"Europe/Lisbon", kli18nc("@item city", "Lisbon")},
    {"Europe/Paris", kli18nc("@item city", "Paris")},
    {"Europe/Paris", kli18nc("@item city", "Lyon")},
    {"Europe/Paris", kli18nc("@item city", "Marseille")},
    {"Europe/Brussels", kli18nc("@item city", "Brussels")},
    {"Europe/Luxembourg", kli18nc("@item city", "Luxembourg")},
    {"Europe/Amsterdam", kli18nc("@item city", "Amsterdam")},
    {"Europe/Zurich", kli18nc("@item city", "Zurich")},
    {"Europe/Zurich", kli18nc("@item city", "Geneva")},
    {"Europe/Berlin", kli18nc("@item city", "Berlin")},
    {"Europe/Madrid", kli18nc("@item city", "Madrid")},
    {"Europe/Madrid", kli18nc("@item city", "Barcelona")},
    {"Europe/Rome", kli18nc("@item city", "Rome")},
    {"Europe/Vienna", kli18nc("@item city", "Vienna")},
    {"Europe/Prague", kli18nc("@item city", "Prague")},
    {"Europe/Warsaw", kli18nc("@item city", "Warsaw")},
    {"Europe/Copenhagen", kli18nc("@item city", "Copenhagen")},
    {"Europe/Oslo", kli18nc("@item city", "Oslo")},
    {"Europe/Stockholm", kli18nc("@item city", "Stockholm")},
    {"Europe/Helsinki", kli18nc("@item city", "Helsinki")},
    {"Europe/Athens", kli18nc("@item city", "Athens")},
    {"Europe/Bucharest", kli18nc("@item city", "Bucharest")},
    {"Europe/Kyiv", kli18nc("@item city", "Kyiv")},
    {"Europe/Istanbul", kli18nc("@item city", "Istanbul")},
    {"Europe/Moscow", kli18nc("@item city", "Moscow")},
    {"Africa/Casablanca", kli18nc("@item city", "Casablanca")},
    {"Africa/Algiers", kli18nc("@item city", "Algiers")},
    {"Africa/Tunis", kli18nc("@item city", "Tunis")},
    {"Africa/Dakar", kli18nc("@item city", "Dakar")},
    {"Africa/Abidjan", kli18nc("@item city", "Abidjan")},
    {"Africa/Lagos", kli18nc("@item city", "Lagos")},
    {"Africa/Kinshasa", kli18nc("@item city", "Kinshasa")},
    {"Africa/Cairo", kli18nc("@item city", "Cairo")},
    {"Africa/Nairobi", kli18nc("@item city", "Nairobi")},
    {"Africa/Johannesburg", kli18nc("@item city", "Johannesburg")},
    {"Indian/Reunion", kli18nc("@item city", "Saint-Denis")},
    {"Indian/Antananarivo", kli18nc("@item city", "Antananarivo")},
    {"Asia/Jerusalem", kli18nc("@item city", "Jerusalem")},
    {"Asia/Beirut", kli18nc("@item city", "Beirut")},
    {"Asia/Riyadh", kli18nc("@item city", "Riyadh")},
    {"Asia/Tehran", kli18nc("@item city", "Tehran")},
    {"Asia/Dubai", kli18nc("@item city", "Dubai")},
    {"Asia/Karachi", kli18nc("@item city", "Karachi")},
    {"Asia/Kolkata", kli18nc("@item city", "New Delhi")},
    {"Asia/Kolkata", kli18nc("@item city", "Mumbai")},
    {"Asia/Kathmandu", kli18nc("@item city", "Kathmandu")},
    {"Asia/Dhaka", kli18nc("@item city", "Dhaka")},
    {"Asia/Bangkok", kli18nc("@item city", "Bangkok")},
    {"Asia/Ho_Chi_Minh", kli18nc("@item city", "Hanoi")},
    {"Asia/Ho_Chi_Minh", kli18nc("@item city", "Ho Chi Minh City")},
    {"Asia/Jakarta", kli18nc("@item city", "Jakarta")},
    {"Asia/Singapore", kli18nc("@item city", "Singapore")},
    {"Asia/Kuala_Lumpur", kli18nc("@item city", "Kuala Lumpur")},
    {"Asia/Manila", kli18nc("@item city", "Manila")},
    {"Asia/Hong_Kong", kli18nc("@item city", "Hong Kong")},
    {"Asia/Shanghai", kli18nc("@item city", "Beijing")},
    {"Asia/Shanghai", kli18nc("@item city", "Shanghai")},
    {"Asia/Taipei", kli18nc("@item city", "Taipei")},
    {"Asia/Seoul", kli18nc("@item city", "Seoul")},
    {"Asia/Tokyo", kli18nc("@item city", "Tokyo")},
    {"Australia/Perth", kli18nc("@item city", "Perth")},
    {"Australia/Darwin", kli18nc("@item city", "Darwin")},
    {"Australia/Adelaide", kli18nc("@item city", "Adelaide")},
    {"Australia/Brisbane", kli18nc("@item city", "Brisbane")},
    {"Australia/Sydney", kli18nc("@item city", "Sydney")},
    {"Australia/Melbourne", kli18nc("@item city", "Melbourne")},
    {"Pacific/Noumea", kli18nc("@item city", "Nouméa")},
    {"Pacific/Tahiti", kli18nc("@item city", "Papeete")},
    {"Pacific/Auckland", kli18nc("@item city", "Auckland")},
    {"Pacific/Fiji", kli18nc("@item city", "Suva")},
};
// clang-format on

const QStringList kRegions{QStringLiteral("Africa"), QStringLiteral("America"), QStringLiteral("Asia"),
                           QStringLiteral("Atlantic"), QStringLiteral("Australia"), QStringLiteral("Europe"),
                           QStringLiteral("Indian"), QStringLiteral("Pacific")};

QString countryOf(const QByteArray &zone)
{
    return KTimeZone::country(zone.constData()).name();
}

City known(const Known &k)
{
    const QByteArray zone(k.zone);
    return City{QString::fromLatin1(zone) + QLatin1Char('|') + QString::fromUtf8(k.name.untranslatedText()), zone,
                k.name.toString(), countryOf(zone), true};
}

City plain(const QByteArray &zone)
{
    return City{QString::fromLatin1(zone), zone, WorldTime::cityOf(zone), countryOf(zone), false};
}

bool isPlace(const QByteArray &zone)
{
    const int slash = zone.indexOf('/');
    return slash > 0 && kRegions.contains(QString::fromLatin1(zone.left(slash)));
}
} // namespace

QList<City> Cities::all()
{
    QList<City> out;
    QSet<QByteArray> named;
    for (const Known &k : kKnown) {
        if (!QTimeZone::isTimeZoneIdAvailable(k.zone))
            continue;
        out << known(k);
        named.insert(k.zone);
    }
    for (const QByteArray &zone : QTimeZone::availableTimeZoneIds()) {
        if (isPlace(zone) && !named.contains(zone))
            out << plain(zone);
    }
    return out;
}

City Cities::of(const QString &key)
{
    const int bar = key.indexOf(QLatin1Char('|'));
    const QByteArray zone = (bar < 0 ? key : key.left(bar)).toLatin1();
    if (!QTimeZone::isTimeZoneIdAvailable(zone))
        return {};
    if (bar >= 0) {
        const QByteArray english = key.mid(bar + 1).toUtf8();
        for (const Known &k : kKnown) {
            if (zone == k.zone && english == k.name.untranslatedText())
                return known(k);
        }
    }
    return plain(zone);
}

// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "holidays.h"

#include <algorithm>

namespace
{
// weekday: 1 Monday ... 7 Sunday; nth from 1, or -1 for the last
QDate nth(int year, int month, int weekday, int n)
{
    if (n > 0) {
        const QDate first(year, month, 1);
        return first.addDays((weekday - first.dayOfWeek() + 7) % 7 + 7 * (n - 1));
    }
    const QDate last(year, month, QDate(year, month, 1).daysInMonth());
    return last.addDays(-((last.dayOfWeek() - weekday + 7) % 7));
}

// the Monday before May 25th (Victoria Day, Journée nationale des patriotes)
QDate mondayBeforeMay25(int year)
{
    const QDate d(year, 5, 24);
    return d.addDays(-((d.dayOfWeek() - 1 + 7) % 7));
}
} // namespace

QStringList Holidays::regions()
{
    return {QStringLiteral("CA-QC"), QStringLiteral("CA"), QStringLiteral("FR"), QStringLiteral("BE"),
            QStringLiteral("CH"), QStringLiteral("US"), QStringLiteral("GB")};
}

QString Holidays::regionFor(const QLocale &locale)
{
    switch (locale.territory()) {
    case QLocale::Canada:
        return locale.language() == QLocale::French ? QStringLiteral("CA-QC") : QStringLiteral("CA");
    case QLocale::France:
        return QStringLiteral("FR");
    case QLocale::Belgium:
        return QStringLiteral("BE");
    case QLocale::Switzerland:
        return QStringLiteral("CH");
    case QLocale::UnitedStates:
        return QStringLiteral("US");
    case QLocale::UnitedKingdom:
        return QStringLiteral("GB");
    default:
        return {};
    }
}

// the Gregorian Easter Sunday (the "anonymous" algorithm)
QDate Holidays::easter(int y)
{
    const int a = y % 19, b = y / 100, c = y % 100, d = b / 4, e = b % 4;
    const int f = (b + 8) / 25, g = (b - f + 1) / 3, h = (19 * a + b - d - g + 15) % 30;
    const int i = c / 4, k = c % 4, l = (32 + 2 * e + 2 * i - h - k) % 7;
    const int m = (a + 11 * h + 22 * l) / 451;
    const int month = (h + l - 7 * m + 114) / 31, day = (h + l - 7 * m + 114) % 31 + 1;
    return QDate(y, month, day);
}

QList<Holiday> Holidays::forYear(const QString &region, int y)
{
    const QDate e = easter(y);
    QList<Holiday> h;
    auto add = [&h](const QDate &d, const char *id) { h << Holiday{d, QString::fromLatin1(id)}; };

    if (region == QLatin1String("CA-QC")) {
        add({y, 1, 1}, "new-year");
        add(e.addDays(-2), "good-friday");
        add(e.addDays(1), "easter-monday");
        add(mondayBeforeMay25(y), "patriotes");
        add({y, 6, 24}, "saint-jean");
        add({y, 7, 1}, "canada-day");
        add(nth(y, 9, 1, 1), "labour-day");
        add(nth(y, 10, 1, 2), "thanksgiving-ca");
        add({y, 12, 25}, "christmas");
    } else if (region == QLatin1String("CA")) {
        add({y, 1, 1}, "new-year");
        add(e.addDays(-2), "good-friday");
        add(mondayBeforeMay25(y), "victoria-day");
        add({y, 7, 1}, "canada-day");
        add(nth(y, 9, 1, 1), "labour-day");
        add({y, 9, 30}, "truth-reconciliation");
        add(nth(y, 10, 1, 2), "thanksgiving-ca");
        add({y, 11, 11}, "remembrance");
        add({y, 12, 25}, "christmas");
        add({y, 12, 26}, "boxing-day");
    } else if (region == QLatin1String("FR")) {
        add({y, 1, 1}, "new-year");
        add(e.addDays(1), "easter-monday");
        add({y, 5, 1}, "may-day");
        add({y, 5, 8}, "victory-1945");
        add(e.addDays(39), "ascension");
        add(e.addDays(50), "whit-monday");
        add({y, 7, 14}, "bastille-day");
        add({y, 8, 15}, "assumption");
        add({y, 11, 1}, "all-saints");
        add({y, 11, 11}, "armistice");
        add({y, 12, 25}, "christmas");
    } else if (region == QLatin1String("BE")) {
        add({y, 1, 1}, "new-year");
        add(e.addDays(1), "easter-monday");
        add({y, 5, 1}, "may-day");
        add(e.addDays(39), "ascension");
        add(e.addDays(50), "whit-monday");
        add({y, 7, 21}, "belgian-national-day");
        add({y, 8, 15}, "assumption");
        add({y, 11, 1}, "all-saints");
        add({y, 11, 11}, "armistice");
        add({y, 12, 25}, "christmas");
    } else if (region == QLatin1String("CH")) {
        add({y, 1, 1}, "new-year");
        add(e.addDays(-2), "good-friday");
        add(e.addDays(1), "easter-monday");
        add(e.addDays(39), "ascension");
        add(e.addDays(50), "whit-monday");
        add({y, 8, 1}, "swiss-national-day");
        add({y, 12, 25}, "christmas");
        add({y, 12, 26}, "saint-stephen");
    } else if (region == QLatin1String("US")) {
        add({y, 1, 1}, "new-year");
        add(nth(y, 1, 1, 3), "mlk-day");
        add(nth(y, 2, 1, 3), "presidents-day");
        add(nth(y, 5, 1, -1), "memorial-day");
        add({y, 6, 19}, "juneteenth");
        add({y, 7, 4}, "independence-day");
        add(nth(y, 9, 1, 1), "labor-day-us");
        add(nth(y, 10, 1, 2), "columbus-day");
        add({y, 11, 11}, "veterans-day");
        add(nth(y, 11, 4, 4), "thanksgiving-us");
        add({y, 12, 25}, "christmas");
    } else if (region == QLatin1String("GB")) {
        add({y, 1, 1}, "new-year");
        add(e.addDays(-2), "good-friday");
        add(e.addDays(1), "easter-monday");
        add(nth(y, 5, 1, 1), "early-may");
        add(nth(y, 5, 1, -1), "spring-bank");
        add(nth(y, 8, 1, -1), "summer-bank");
        add({y, 12, 25}, "christmas");
        add({y, 12, 26}, "boxing-day");
    }
    std::sort(h.begin(), h.end(), [](const Holiday &a, const Holiday &b) { return a.date < b.date; });
    return h;
}

QList<Holiday> Holidays::between(const QString &region, const QDate &from, const QDate &to)
{
    QList<Holiday> out;
    if (region.isEmpty() || !from.isValid() || !to.isValid())
        return out;
    for (int y = from.year(); y <= to.year(); ++y) {
        for (const Holiday &h : forYear(region, y)) {
            if (h.date >= from && h.date <= to)
                out << h;
        }
    }
    return out;
}

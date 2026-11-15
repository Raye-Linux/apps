// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "holidaynames.h"

#include <KLocalizedString>

#include <QHash>

QString holidayName(const QString &id)
{
    static const QHash<QString, KLocalizedString> names{
        {QStringLiteral("new-year"), ki18nc("@info holiday", "New Year's Day")},
        {QStringLiteral("good-friday"), ki18nc("@info holiday", "Good Friday")},
        {QStringLiteral("easter-monday"), ki18nc("@info holiday", "Easter Monday")},
        {QStringLiteral("patriotes"), ki18nc("@info holiday", "National Patriots' Day")},
        {QStringLiteral("victoria-day"), ki18nc("@info holiday", "Victoria Day")},
        {QStringLiteral("saint-jean"), ki18nc("@info holiday", "Saint-Jean-Baptiste Day")},
        {QStringLiteral("canada-day"), ki18nc("@info holiday", "Canada Day")},
        {QStringLiteral("labour-day"), ki18nc("@info holiday", "Labour Day")},
        {QStringLiteral("truth-reconciliation"), ki18nc("@info holiday", "Truth and Reconciliation Day")},
        {QStringLiteral("thanksgiving-ca"), ki18nc("@info holiday", "Thanksgiving")},
        {QStringLiteral("remembrance"), ki18nc("@info holiday", "Remembrance Day")},
        {QStringLiteral("christmas"), ki18nc("@info holiday", "Christmas Day")},
        {QStringLiteral("boxing-day"), ki18nc("@info holiday", "Boxing Day")},
        {QStringLiteral("may-day"), ki18nc("@info holiday", "Labour Day (May 1)")},
        {QStringLiteral("victory-1945"), ki18nc("@info holiday", "Victory in Europe Day")},
        {QStringLiteral("ascension"), ki18nc("@info holiday", "Ascension Day")},
        {QStringLiteral("whit-monday"), ki18nc("@info holiday", "Whit Monday")},
        {QStringLiteral("bastille-day"), ki18nc("@info holiday", "Bastille Day")},
        {QStringLiteral("assumption"), ki18nc("@info holiday", "Assumption Day")},
        {QStringLiteral("all-saints"), ki18nc("@info holiday", "All Saints' Day")},
        {QStringLiteral("armistice"), ki18nc("@info holiday", "Armistice Day")},
        {QStringLiteral("belgian-national-day"), ki18nc("@info holiday", "Belgian National Day")},
        {QStringLiteral("swiss-national-day"), ki18nc("@info holiday", "Swiss National Day")},
        {QStringLiteral("saint-stephen"), ki18nc("@info holiday", "St. Stephen's Day")},
        {QStringLiteral("mlk-day"), ki18nc("@info holiday", "Martin Luther King Jr. Day")},
        {QStringLiteral("presidents-day"), ki18nc("@info holiday", "Presidents' Day")},
        {QStringLiteral("memorial-day"), ki18nc("@info holiday", "Memorial Day")},
        {QStringLiteral("juneteenth"), ki18nc("@info holiday", "Juneteenth")},
        {QStringLiteral("independence-day"), ki18nc("@info holiday", "Independence Day")},
        {QStringLiteral("labor-day-us"), ki18nc("@info holiday", "Labor Day")},
        {QStringLiteral("columbus-day"), ki18nc("@info holiday", "Columbus Day")},
        {QStringLiteral("veterans-day"), ki18nc("@info holiday", "Veterans Day")},
        {QStringLiteral("thanksgiving-us"), ki18nc("@info holiday", "Thanksgiving Day")},
        {QStringLiteral("early-may"), ki18nc("@info holiday", "Early May bank holiday")},
        {QStringLiteral("spring-bank"), ki18nc("@info holiday", "Spring bank holiday")},
        {QStringLiteral("summer-bank"), ki18nc("@info holiday", "Summer bank holiday")},
    };
    const auto it = names.constFind(id);
    return it == names.cend() ? id : it->toString();
}

QString holidayRegionName(const QString &region)
{
    if (region == QLatin1String("CA-QC"))
        return i18nc("@item holidays of", "Canada (Quebec)");
    if (region == QLatin1String("CA"))
        return i18nc("@item holidays of", "Canada");
    if (region == QLatin1String("FR"))
        return i18nc("@item holidays of", "France");
    if (region == QLatin1String("BE"))
        return i18nc("@item holidays of", "Belgium");
    if (region == QLatin1String("CH"))
        return i18nc("@item holidays of", "Switzerland");
    if (region == QLatin1String("US"))
        return i18nc("@item holidays of", "United States");
    if (region == QLatin1String("GB"))
        return i18nc("@item holidays of", "United Kingdom");
    return region;
}

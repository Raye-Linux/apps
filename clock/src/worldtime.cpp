// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "worldtime.h"

#include <QRegularExpression>

#include <algorithm>

WorldTime::Gap WorldTime::compare(const QTimeZone &place, const QTimeZone &here, const QDateTime &now)
{
    Gap g;
    g.minutes = (place.offsetFromUtc(now) - here.offsetFromUtc(now)) / 60;
    g.days = int(now.toTimeZone(here).date().daysTo(now.toTimeZone(place).date()));
    return g;
}

bool WorldTime::daytime(const QTimeZone &place, const QDateTime &now)
{
    const int h = now.toTimeZone(place).time().hour();
    return h >= 6 && h < 19;
}

QString WorldTime::fold(const QString &text)
{
    QString out;
    const QString decomposed = text.normalized(QString::NormalizationForm_KD);
    out.reserve(decomposed.size());
    for (const QChar c : decomposed) {
        if (c.category() == QChar::Mark_NonSpacing)
            continue;
        out += c == QLatin1Char('-') || c == QLatin1Char('_') || c == QLatin1Char('\'') ? QLatin1Char(' ') : c.toLower();
    }
    return out;
}

bool WorldTime::matches(const QString &query, const QStringList &fields)
{
    static const QRegularExpression space(QStringLiteral("\\s+"));
    const QStringList words = fold(query).split(space, Qt::SkipEmptyParts);
    if (words.isEmpty())
        return true;
    QStringList fieldWords;
    for (const QString &f : fields)
        fieldWords += fold(f).split(space, Qt::SkipEmptyParts);
    for (const QString &w : words) {
        const bool found = std::any_of(fieldWords.cbegin(), fieldWords.cend(), [&w](const QString &fw) { return fw.startsWith(w); });
        if (!found)
            return false;
    }
    return true;
}

QString WorldTime::cityOf(const QByteArray &zoneId)
{
    const QByteArray last = zoneId.mid(zoneId.lastIndexOf('/') + 1);
    return QString::fromUtf8(last).replace(QLatin1Char('_'), QLatin1Char(' '));
}

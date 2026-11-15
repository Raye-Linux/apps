// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Public holidays, worked out for any year: fixed days, the "nth Monday"
// kind and the ones that follow Easter. Off unless chosen; the names are
// in holidaynames.cpp, with the words of the apps.

#pragma once

#include <QDate>
#include <QList>
#include <QLocale>
#include <QStringList>

struct Holiday
{
    QDate date;
    QString id;
};

namespace Holidays
{
QStringList regions();      // "CA-QC", "CA", "FR", "BE", "CH", "US", "GB"
QString regionFor(const QLocale &locale);
QList<Holiday> forYear(const QString &region, int year);
QList<Holiday> between(const QString &region, const QDate &from, const QDate &to);   // [from, to]
QDate easter(int year);
}

// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "worldtime.h"

#include <QTest>

namespace
{
const QTimeZone toronto("America/Toronto");
const QTimeZone paris("Europe/Paris");
const QTimeZone tokyo("Asia/Tokyo");
const QTimeZone kolkata("Asia/Kolkata");

QDateTime utc(int month, int day, int h, int m = 0)
{
    return QDateTime(QDate(2026, month, day), QTime(h, m), QTimeZone::UTC);
}
} // namespace

class TestWorldTime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void hours_ahead_and_behind()
    {
        QCOMPARE(WorldTime::compare(paris, toronto, utc(11, 18, 12)).minutes, 6 * 60);
        QCOMPARE(WorldTime::compare(toronto, paris, utc(11, 18, 12)).minutes, -6 * 60);
        QCOMPARE(WorldTime::compare(kolkata, toronto, utc(11, 18, 12)).minutes, 10 * 60 + 30);
    }

    void gap_moves_with_daylight_saving()
    {
        // Paris changes on 2026-10-25, Toronto on 2026-11-01: a week at five hours
        QCOMPARE(WorldTime::compare(paris, toronto, utc(10, 20, 12)).minutes, 6 * 60);
        QCOMPARE(WorldTime::compare(paris, toronto, utc(10, 28, 12)).minutes, 5 * 60);
        QCOMPARE(WorldTime::compare(paris, toronto, utc(11, 3, 12)).minutes, 6 * 60);
    }

    void tomorrow_and_yesterday()
    {
        // 22:00 in Toronto is 12:00 the next day in Tokyo
        QCOMPARE(WorldTime::compare(tokyo, toronto, utc(11, 19, 3)).days, 1);
        QCOMPARE(WorldTime::compare(toronto, tokyo, utc(11, 19, 3)).days, -1);
        QCOMPARE(WorldTime::compare(paris, toronto, utc(11, 18, 12)).days, 0);
    }

    void day_and_night()
    {
        QVERIFY(WorldTime::daytime(paris, utc(11, 18, 12)));
        QVERIFY(!WorldTime::daytime(paris, utc(11, 18, 23)));
    }

    void search_folds_accents_and_case()
    {
        QCOMPARE(WorldTime::fold(QStringLiteral("Montréal")), QStringLiteral("montreal"));
        QVERIFY(WorldTime::matches(QStringLiteral("montre"), {QStringLiteral("Montréal"), QStringLiteral("Canada")}));
        QVERIFY(WorldTime::matches(QStringLiteral("new york"), {QStringLiteral("New York"), QStringLiteral("United States")}));
        QVERIFY(WorldTime::matches(QStringLiteral("york"), {QStringLiteral("New York")}));
        QVERIFY(!WorldTime::matches(QStringLiteral("ork"), {QStringLiteral("New York")}));
        QVERIFY(WorldTime::matches(QStringLiteral("  "), {QStringLiteral("Paris")}));
        QVERIFY(WorldTime::matches(QStringLiteral("saint jean"), {QStringLiteral("St. John's"), QStringLiteral("Saint-Jean")}));
    }

    void city_from_zone()
    {
        QCOMPARE(WorldTime::cityOf("Europe/Paris"), QStringLiteral("Paris"));
        QCOMPARE(WorldTime::cityOf("America/Argentina/Buenos_Aires"), QStringLiteral("Buenos Aires"));
        QCOMPARE(WorldTime::cityOf("UTC"), QStringLiteral("UTC"));
    }
};

QTEST_GUILESS_MAIN(TestWorldTime)
#include "test_worldtime.moc"

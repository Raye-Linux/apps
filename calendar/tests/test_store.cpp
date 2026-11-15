// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "calstore.h"
#include "holidays.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>
#include <QTimeZone>

#include <csignal>
#include <sys/resource.h>

class TestStore : public QObject
{
    Q_OBJECT

private:
    static CalEvent meeting(const QDateTime &start)
    {
        CalEvent e;
        e.uid = QStringLiteral("s");
        e.summary = QStringLiteral("Call");
        e.start = start;
        e.end = start.addSecs(600);
        e.hasAlarm = true;
        e.alarmMinutes = 5;
        return e;
    }

private Q_SLOTS:
    void saveIsPrivateAndReadable()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("new/calendar.ics"));
        CalFile f;
        f.events << meeting(QDateTime(QDate(2026, 11, 11), QTime(15, 0), QTimeZone::UTC));
        QVERIFY(CalStore::save(path, f));
        // the user alone: the dates of a life are nobody else's business
        QCOMPARE(QFileInfo(path).permissions() & 0x0777, QFileDevice::ReadUser | QFileDevice::WriteUser);
        CalFile back;
        QVERIFY(CalStore::load(path, &back));
        QCOMPARE(back.events.first().summary, QStringLiteral("Call"));
    }

    void failedSaveKeepsTheOldFile()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("calendar.ics"));
        CalFile f;
        f.events << meeting(QDateTime(QDate(2026, 11, 11), QTime(15, 0), QTimeZone::UTC));
        QVERIFY(CalStore::save(path, f));
        QFile r(path);
        QVERIFY(r.open(QIODevice::ReadOnly));
        const QByteArray before = r.readAll();
        r.close();

        // a disk that fills up half way through (root obeys this limit too)
        f.events.first().summary = QString(200000, QLatin1Char('x'));
        ::signal(SIGXFSZ, SIG_IGN);
        rlimit old{};
        ::getrlimit(RLIMIT_FSIZE, &old);
        rlimit small = old;
        small.rlim_cur = 8192;
        ::setrlimit(RLIMIT_FSIZE, &small);
        QString error;
        const bool saved = CalStore::save(path, f, &error);
        ::setrlimit(RLIMIT_FSIZE, &old);

        QVERIFY(!saved);
        QVERIFY(!error.isEmpty());
        QVERIFY(r.open(QIODevice::ReadOnly));
        QCOMPARE(r.readAll(), before);
        // and no half-written temporary file next to it
        QCOMPARE(QDir(dir.path()).entryList(QDir::Files | QDir::Hidden), QStringList{QStringLiteral("calendar.ics")});
    }

    void scheduleWritesOneTimer()
    {
        QTemporaryDir dir;
        const QString timer = dir.filePath(QStringLiteral("systemd/user/raye-calendar-reminder.timer"));
        QStringList calls;
        const auto fake = [&calls](const QStringList &a) { calls << a.join(QLatin1Char(' ')); return true; };
        const QDateTime now(QDate(2026, 11, 11), QTime(14, 0), QTimeZone::UTC);
        const QList<CalEvent> ev{meeting(now.addSecs(7 * 60))};

        QCOMPARE(Schedule::apply(ev, {}, now, timer, fake), Schedule::Action::Timer);
        QFile f(timer);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QVERIFY(f.readAll().contains("OnCalendar=2026-11-11 14:02:00 UTC"));
        QVERIFY(QFileInfo(dir.filePath(QStringLiteral("systemd/user/timers.target.wants/raye-calendar-reminder.timer"))).isSymLink());
        QCOMPARE(calls, (QStringList{QStringLiteral("daemon-reload"), QStringLiteral("restart raye-calendar-reminder.timer")}));

        // the same time again: no reload of the manager
        calls.clear();
        Schedule::apply(ev, {}, now.addSecs(30), timer, fake);
        QCOMPARE(calls, QStringList{QStringLiteral("restart raye-calendar-reminder.timer")});

        // due: the reminder service starts now, no waiting for a timer
        calls.clear();
        QCOMPARE(Schedule::apply(ev, {}, now.addSecs(150), timer, fake), Schedule::Action::StartNow);
        QCOMPARE(calls, QStringList{QStringLiteral("start --no-block raye-calendar-reminder.service")});

        // nothing left: the timer goes away
        calls.clear();
        QCOMPARE(Schedule::apply({}, {}, now, timer, fake), Schedule::Action::Nothing);
        QVERIFY(!QFile::exists(timer));
        QVERIFY(!QFileInfo(dir.filePath(QStringLiteral("systemd/user/timers.target.wants/raye-calendar-reminder.timer"))).isSymLink());
        QCOMPARE(calls.first(), QStringLiteral("stop raye-calendar-reminder.timer"));
    }

    void holidays()
    {
        QCOMPARE(Holidays::easter(2026), QDate(2026, 4, 5));
        QCOMPARE(Holidays::easter(2027), QDate(2027, 3, 28));
        const auto qc = Holidays::forYear(QStringLiteral("CA-QC"), 2026);
        auto has = [](const QList<Holiday> &l, const char *id, QDate d) {
            for (const auto &h : l)
                if (h.id == QLatin1String(id))
                    return h.date == d;
            return false;
        };
        QVERIFY(has(qc, "patriotes", QDate(2026, 5, 18)));
        QVERIFY(has(qc, "saint-jean", QDate(2026, 6, 24)));
        QVERIFY(has(qc, "thanksgiving-ca", QDate(2026, 10, 12)));
        const auto us = Holidays::forYear(QStringLiteral("US"), 2026);
        QVERIFY(has(us, "thanksgiving-us", QDate(2026, 11, 26)));
        QVERIFY(has(us, "memorial-day", QDate(2026, 5, 25)));
        QVERIFY(has(Holidays::forYear(QStringLiteral("FR"), 2026), "ascension", QDate(2026, 5, 14)));
        QCOMPARE(Holidays::regionFor(QLocale(QLocale::French, QLocale::Canada)), QStringLiteral("CA-QC"));
        QVERIFY(Holidays::between(QString(), QDate(2026, 1, 1), QDate(2026, 12, 31)).isEmpty());
    }

    // a moved meeting carries its series' UID: it must not replace the series
    void importKeepsTheSeries()
    {
        CalFile mine;
        CalFile in;
        CalEvent series;
        series.uid = QStringLiteral("weekly@x");
        series.summary = QStringLiteral("Standup");
        series.rule.freq = Rule::Weekly;
        CalEvent moved = series;
        moved.rule = Rule{};
        moved.extra << QStringLiteral("RECURRENCE-ID:20261118T090000Z");
        CalEvent other;
        other.uid = QStringLiteral("one@x");
        in.events << series << moved << other;
        int leftOut = -1;
        QCOMPARE(CalStore::mergeImport(mine, in, &leftOut), 2);
        QCOMPARE(leftOut, 1);
        QCOMPARE(mine.events.size(), 2);
        QCOMPARE(mine.events.at(0).rule.freq, Rule::Weekly);
        // and the same file again updates, never doubles
        QCOMPARE(CalStore::mergeImport(mine, in), 2);
        QCOMPARE(mine.events.size(), 2);
    }
};

QTEST_GUILESS_MAIN(TestStore)
#include "test_store.moc"

// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The settings file, the checks the root helper runs on it, and the
// exact sysfs writes that follow, on each kind of card.

#include "fakecards.h"
#include "tuning.h"

#include <QTemporaryDir>
#include <QTest>

using namespace Raye::Graphics;

namespace
{
struct Setup {
    Card card;
    Limits limits;
};
Setup setup(const QString &sys)
{
    const Card c = findCards(sys).value(0);
    return {c, readLimits(c)};
}
QList<CurvePoint> curve5()
{
    return {{40, 25}, {55, 35}, {65, 50}, {75, 75}, {82, 100}};
}
QStringList shown(const QList<Write> &writes, const QString &root)
{
    QStringList out;
    for (const Write &w : writes)
        out.append(QString(w.file).remove(root) + " < " + QString::fromLatin1(w.value));
    return out;
}
} // namespace

class TestTuning : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fileRoundTrip()
    {
        Config c;
        c.performance = "high";
        c.profile = 1;
        c.powerCapW = 120;
        c.fanCurve = true;
        c.curve = curve5();
        const auto back = parseConfig(writeConfig(c));
        QVERIFY(back);
        QCOMPARE(*back, c);
        const auto empty = parseConfig("");
        QVERIFY(empty);
        QVERIFY(empty->isDefault());
    }

    void fileRefusals_data()
    {
        QTest::addColumn<QByteArray>("text");
        QTest::newRow("unknown key") << QByteArray("voltage=1200\n");
        QTest::newRow("twice") << QByteArray("power-cap=100\npower-cap=120\n");
        QTest::newRow("level") << QByteArray("performance=manual\n");
        QTest::newRow("not a number") << QByteArray("power-cap=12e4\n");
        QTest::newRow("huge number") << QByteArray("power-cap=12345678\n");
        QTest::newRow("fan") << QByteArray("fan=max\n");
        QTest::newRow("curve") << QByteArray("curve=40:20,55\n");
        QTest::newRow("long curve") << QByteArray("curve=1:1,2:2,3:3,4:4,5:5,6:6,7:7,8:8,9:9\n");
        QTest::newRow("no equals") << QByteArray("high\n");
        QTest::newRow("too long") << QByteArray(MaxConfigSize + 1, '#');
    }
    void fileRefusals()
    {
        QFETCH(QByteArray, text);
        QString why;
        QVERIFY(!parseConfig(text, &why));
        QVERIFY(!why.isEmpty());
    }

    void checks()
    {
        QTemporaryDir dir;
        Fake::polaris(dir.path());
        const Limits l = setup(dir.path()).limits;
        Config c;
        QVERIFY(checkConfig(c, l).isEmpty());
        c.powerCapW = 176;
        QVERIFY(!checkConfig(c, l).isEmpty());
        c.powerCapW = 69;
        QVERIFY(!checkConfig(c, l).isEmpty());
        c.powerCapW = 70;
        QVERIFY(checkConfig(c, l).isEmpty());
        c.powerCapW = 175;
        QVERIFY(checkConfig(c, l).isEmpty());
        c.profile = 6; // CUSTOM
        QVERIFY(!checkConfig(c, l).isEmpty());
        c.profile = 9;
        QVERIFY(!checkConfig(c, l).isEmpty());
        c.profile = -2;
        QVERIFY(!checkConfig(c, l).isEmpty());
        c.profile = 5;
        QVERIFY(checkConfig(c, l).isEmpty());

        c.fanCurve = true;
        c.curve = curve5();
        QVERIFY(checkConfig(c, l).isEmpty());
        c.curve[4].temp = 85; // above crit 94 - 10
        QVERIFY(!checkConfig(c, l).isEmpty());
        c.curve = curve5();
        c.curve[2].speed = 30; // goes down
        QVERIFY(!checkConfig(c, l).isEmpty());
        c.curve = curve5();
        c.curve[2].temp = 55; // same temperature twice
        QVERIFY(!checkConfig(c, l).isEmpty());
        c.curve = {{40, 30}};
        QVERIFY(!checkConfig(c, l).isEmpty());
        c.curve = {{40, 101}, {50, 101}};
        QVERIFY(!checkConfig(c, l).isEmpty());
        c.curve = {{40, 30}, {80, 100}};
        QVERIFY(checkConfig(c, l).isEmpty());
    }

    void integratedRefusesFanAndPower()
    {
        QTemporaryDir dir;
        Fake::cezanne(dir.path());
        const Limits l = setup(dir.path()).limits;
        Config c;
        c.performance = "low";
        QVERIFY(checkConfig(c, l).isEmpty());
        c.powerCapW = 15;
        QVERIFY(!checkConfig(c, l).isEmpty());
        c.powerCapW = 0;
        c.fanCurve = true;
        c.curve = curve5();
        QVERIFY(!checkConfig(c, l).isEmpty());
    }

    void firmwareCurveRules()
    {
        QTemporaryDir dir;
        Fake::navi31(dir.path());
        const Limits l = setup(dir.path()).limits;
        Config c;
        c.fanCurve = true;
        c.curve = curve5();
        QVERIFY(checkConfig(c, l).isEmpty());
        c.curve = {{40, 30}, {80, 100}}; // the card takes exactly five
        QVERIFY(!checkConfig(c, l).isEmpty());
        c.curve = curve5();
        c.curve[0].speed = 10; // under the card's 15 %
        QVERIFY(!checkConfig(c, l).isEmpty());
        c.curve = curve5();
        c.curve[0].temp = 20; // under the card's 25 C
        QVERIFY(!checkConfig(c, l).isEmpty());
        c.curve = curve5();
        c.powerCapW = 300; // under the card's 305 W
        QVERIFY(!checkConfig(c, l).isEmpty());
    }

    void defaultCurvePassesTheChecks()
    {
        QTemporaryDir dir;
        Fake::polaris(dir.path(), 0, "0000:01:00.0");
        Fake::navi21(dir.path(), 1, "0000:03:00.0");
        Fake::navi31(dir.path(), 2, "0000:0c:00.0");
        for (const Card &card : findCards(dir.path())) {
            const Limits l = readLimits(card);
            Config c;
            c.fanCurve = true;
            c.curve = defaultCurve(l);
            QVERIFY2(checkConfig(c, l).isEmpty(), qPrintable(card.slot + ": " + checkConfig(c, l)));
            QCOMPARE(c.curve.constLast().speed, 100);
        }
    }

    void polarisApply()
    {
        QTemporaryDir dir;
        Fake::polaris(dir.path());
        const Setup s = setup(dir.path());
        Config c;
        c.performance = "high";
        c.powerCapW = 120;
        c.fanCurve = true;
        c.curve = curve5();
        const QString d = "/class/drm/card0/device";
        QCOMPARE(shown(planApply(s.card, s.limits, c), dir.path()),
                 QStringList({d + "/power_dpm_force_performance_level < manual",
                              d + "/pp_power_profile_mode < 0",
                              d + "/power_dpm_force_performance_level < high",
                              d + "/hwmon/hwmon2/power1_cap < 120000000"}));
        // its fan has no curve of its own: the service follows it
        QVERIFY(needsFanService(s.limits, c));
    }

    void profileGoesThroughManual()
    {
        QTemporaryDir dir;
        Fake::polaris(dir.path());
        const Setup s = setup(dir.path());
        Config c;
        c.profile = 5;
        const QString d = "/class/drm/card0/device";
        QCOMPARE(shown(planApply(s.card, s.limits, c), dir.path()),
                 QStringList({d + "/power_dpm_force_performance_level < manual",
                              d + "/pp_power_profile_mode < 5"}));
        // the profile already active: nothing to write but the level
        c.profile = 1;
        QCOMPARE(shown(planApply(s.card, s.limits, c), dir.path()),
                 QStringList({d + "/power_dpm_force_performance_level < manual"}));
    }

    void polarisResetUsesTheSavedLimit()
    {
        QTemporaryDir dir;
        const Fake::Tree t = Fake::polaris(dir.path());
        Fake::put(t.hw + "/power1_cap", "120000000\n");
        Fake::put(t.hw + "/pwm1_enable", "1\n");
        Fake::put(t.dev + "/power_dpm_force_performance_level", "high\n");
        Fake::put(t.dev + "/pp_power_profile_mode", QByteArray(Fake::polarisProfiles()).replace("3D_FULL_SCREEN *", "3D_FULL_SCREEN  ").replace("BOOTUP_DEFAULT:", "BOOTUP_DEFAULT*:"));
        const Setup s = setup(dir.path());
        const QString d = "/class/drm/card0/device";
        QCOMPARE(shown(planReset(s.card, s.limits, 145), dir.path()),
                 QStringList({d + "/power_dpm_force_performance_level < auto",
                              d + "/hwmon/hwmon2/power1_cap < 145000000",
                              d + "/hwmon/hwmon2/pwm1_enable < 2"}));
        // without a saved limit the card keeps the one it has
        QCOMPARE(planReset(s.card, s.limits, 0).size(), 2);
    }

    void navi21BackToAutomatic()
    {
        QTemporaryDir dir;
        Fake::navi21(dir.path());
        const Setup s = setup(dir.path());
        const QString d = "/class/drm/card0/device";
        QCOMPARE(shown(planApply(s.card, s.limits, Config()), dir.path()),
                 QStringList({d + "/power_dpm_force_performance_level < manual",
                              d + "/pp_power_profile_mode < 0",
                              d + "/power_dpm_force_performance_level < auto",
                              d + "/hwmon/hwmon2/pwm1_enable < 2"}));
        // the default limit comes from the card itself
        Fake::put(s.card.hwmon + "/power1_cap", "200000000\n");
        const Setup again = setup(dir.path());
        QVERIFY(shown(planReset(again.card, again.limits, 0), dir.path()).contains(d + "/hwmon/hwmon2/power1_cap < 255000000"));
    }

    void navi31FirmwareCurve()
    {
        QTemporaryDir dir;
        Fake::navi31(dir.path());
        const Setup s = setup(dir.path());
        Config c;
        c.profile = 1;
        c.fanCurve = true;
        c.curve = curve5();
        const QString f = "/class/drm/card0/device/gpu_od/fan_ctrl/fan_curve";
        QCOMPARE(shown(planApply(s.card, s.limits, c), dir.path()),
                 QStringList({"/class/drm/card0/device/power_dpm_force_performance_level < manual",
                              f + " < 0 40 25", f + " < 1 55 35", f + " < 2 65 50", f + " < 3 75 75",
                              f + " < 4 82 100", f + " < c"}));
        QVERIFY(!needsFanService(s.limits, c));
        c.fanCurve = false;
        const QStringList back = shown(planApply(s.card, s.limits, c), dir.path());
        QCOMPARE(back.mid(back.size() - 2), QStringList({f + " < r", f + " < c"}));
    }

    void writesStayInTheCard()
    {
        // every file a plan names is under the card's own folders
        QTemporaryDir dir;
        Fake::polaris(dir.path(), 0, "0000:01:00.0");
        Fake::navi21(dir.path(), 1, "0000:03:00.0");
        Fake::navi31(dir.path(), 2, "0000:0c:00.0");
        Fake::cezanne(dir.path(), 3, "0000:05:00.0");
        for (const Card &card : findCards(dir.path())) {
            const Limits l = readLimits(card);
            Config c;
            c.performance = "low";
            c.profile = l.profiles.isEmpty() ? -1 : 2;
            c.powerCapW = l.hasPowerCap ? l.capMaxW : 0;
            c.fanCurve = l.canCurve();
            if (c.fanCurve)
                c.curve = defaultCurve(l);
            QVERIFY(checkConfig(c, l).isEmpty());
            for (const QList<Write> &plan : {planApply(card, l, c), planReset(card, l, 100)})
                for (const Write &w : plan) {
                    QVERIFY2(w.file.startsWith(card.device + "/"), qPrintable(w.file));
                    QVERIFY(!w.file.contains(".."));
                }
        }
    }
};

QTEST_GUILESS_MAIN(TestTuning)
#include "test_tuning.moc"

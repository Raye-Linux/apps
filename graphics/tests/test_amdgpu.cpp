// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// What amdgpu's files say, as four real cards write them.

#include "amdgpu.h"
#include "fakecards.h"

#include <QTemporaryDir>
#include <QTest>

using namespace Raye::Graphics;

namespace
{
Card only(const QString &sys)
{
    const QList<Card> cards = findCards(sys);
    return cards.value(0);
}
} // namespace

class TestAmdgpu : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void dpmLevels()
    {
        const QList<DpmLevel> l = parseDpm("0: 300Mhz\n1: 1366Mhz *\nS: 19Mhz\n");
        QCOMPARE(l.size(), 3);
        QCOMPARE(l.at(1).mhz, 1366);
        QVERIFY(l.at(1).active);
        QVERIFY(!l.at(0).active);
        QCOMPARE(l.at(2).level, -1);
        QVERIFY(parseDpm("").isEmpty());
        QVERIFY(parseDpm("garbage\n").isEmpty());
    }

    void profilesInRows()
    {
        const QList<PowerProfile> p = parseProfiles(Fake::polarisProfiles());
        QCOMPARE(p.size(), 7);
        QCOMPARE(p.at(0).name, "BOOTUP_DEFAULT");
        QCOMPARE(p.at(1).name, "3D_FULL_SCREEN");
        QVERIFY(p.at(1).active);
        QCOMPARE(p.at(6).name, "CUSTOM");
        QCOMPARE(p.at(6).index, 6);
    }

    void profilesWithStarOnTheName()
    {
        QTemporaryDir dir;
        const Fake::Tree t = Fake::navi21(dir.path());
        QFile f(t.dev + "/pp_power_profile_mode");
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QList<PowerProfile> p = parseProfiles(f.readAll());
        // the GFXCLK and SOCCLK rows under each profile are not profiles
        QCOMPARE(p.size(), 7);
        QVERIFY(p.at(1).active);
        QVERIFY(!p.at(0).active);
        QCOMPARE(p.at(2).name, "POWER_SAVING");
    }

    void profilesInColumns()
    {
        const QList<PowerProfile> p = parseProfiles(
            "                              0 BOOTUP_DEFAULT  1 3D_FULL_SCREEN* 2 POWER_SAVING    7 WINDOW_3D       \n"
            "0(       GFXCLK)       0       5       1\n");
        QCOMPARE(p.size(), 4);
        QCOMPARE(p.at(1).name, "3D_FULL_SCREEN");
        QVERIFY(p.at(1).active);
        QCOMPARE(p.at(3).index, 7);
        QCOMPARE(p.at(3).name, "WINDOW_3D");

        const QList<PowerProfile> bare = parseProfiles("   BOOTUP_DEFAULT  3D_FULL_SCREEN* POWER_SAVING\n");
        QCOMPARE(bare.size(), 3);
        QCOMPARE(bare.at(2).index, 2);
        QVERIFY(bare.at(1).active);
    }

    void odFanCurve()
    {
        const OdFanCurve c = parseOdFanCurve("OD_FAN_CURVE:\n0: 30C 20%\n1: 45C 30%\n2: 60C 45%\n3: 75C 70%\n4: 90C 100%\n"
                                             "OD_RANGE:\nFAN_CURVE(hotspot temp): 25C 100C\nFAN_CURVE(fan speed): 15% 100%\n");
        QVERIFY(c.valid);
        QCOMPARE(c.points.size(), 5);
        QCOMPARE(c.points.at(3), (CurvePoint{75, 70}));
        QCOMPARE(c.tempMin, 25);
        QCOMPARE(c.speedMin, 15);
        // no range, or points out of order: not a curve to trust
        QVERIFY(!parseOdFanCurve("OD_FAN_CURVE:\n0: 30C 20%\n1: 45C 30%\n").valid);
        QVERIFY(!parseOdFanCurve("OD_FAN_CURVE:\n1: 30C 20%\n0: 45C 30%\nOD_RANGE:\n"
                                 "FAN_CURVE(hotspot temp): 25C 100C\nFAN_CURVE(fan speed): 15% 100%\n").valid);
    }

    void polarisReading()
    {
        QTemporaryDir dir;
        Fake::polaris(dir.path());
        const Reading r = readReading(only(dir.path()));
        QCOMPARE(r.busy, 12);
        QCOMPARE(r.memBusy, -1);
        QCOMPARE(r.vramTotal, 8589934592ll);
        QCOMPARE(r.sclk, 1366);
        QCOMPARE(r.mclk, 2000);
        QCOMPARE(r.edge, 45.0);
        QVERIFY(std::isnan(r.junction));
        QCOMPARE(curveTemperature(r), 45.0);
        QCOMPARE(r.fanRpm, 1200);
        QCOMPARE(r.fanPercent, 31);
        QCOMPARE(r.powerW, 35.0);
    }

    void polarisLimits()
    {
        QTemporaryDir dir;
        Fake::polaris(dir.path());
        const Limits l = readLimits(only(dir.path()));
        QCOMPARE(l.level, "auto");
        QCOMPARE(l.profiles.size(), 7);
        QCOMPARE(l.defaultProfile, 0);
        QVERIFY(l.hasPowerCap);
        QCOMPARE(l.capW, 145);
        // 0 W on the card, 40 % of the top offered
        QCOMPARE(l.capMinW, 70);
        QCOMPARE(l.capMaxW, 175);
        QCOMPARE(l.capDefaultW, -1);
        QVERIFY(l.hasFan);
        QVERIFY(l.pwmControl);
        QVERIFY(!l.odCurve.valid);
        QVERIFY(!l.junctionSensor);
        QCOMPARE(l.tempCrit, 94.0);
        QCOMPARE(l.curveTempMax(), 84);
        QCOMPARE(l.curvePoints(), 5);
        QVERIFY(l.canCurve());
    }

    void navi21ReadingAndLimits()
    {
        QTemporaryDir dir;
        Fake::navi21(dir.path());
        const Card c = only(dir.path());
        const Reading r = readReading(c);
        QCOMPARE(r.memBusy, 41);
        // no freq files: the starred dpm line
        QCOMPARE(r.sclk, 2250);
        QCOMPARE(r.mclk, 1000);
        QCOMPARE(r.junction, 88.0);
        QCOMPARE(r.memory, 76.0);
        QCOMPARE(curveTemperature(r), 88.0);
        // power1_input when there is no power1_average
        QCOMPARE(r.powerW, 281.0);
        QCOMPARE(r.fanPercent, 60);

        const Limits l = readLimits(c);
        QCOMPARE(l.level, "manual");
        QVERIFY(l.junctionSensor);
        QCOMPARE(l.tempCrit, 110.0);
        QCOMPARE(l.curveTempMax(), 100);
        QCOMPARE(l.capDefaultW, 255);
        QCOMPARE(l.pwmMode, 1);
        QVERIFY(l.pwmControl);
    }

    void navi31UsesTheFirmwareCurve()
    {
        QTemporaryDir dir;
        Fake::navi31(dir.path());
        const Limits l = readLimits(only(dir.path()));
        QVERIFY(l.odCurve.valid);
        QVERIFY(!l.pwmControl);
        QVERIFY(l.canCurve());
        QCOMPARE(l.curvePoints(), 5);
        QCOMPARE(l.curveTempMin(), 25);
        QCOMPARE(l.curveTempMax(), 100);
        QCOMPARE(l.curveSpeedMin(), 15);
        QCOMPARE(l.profiles.size(), 8);
        QCOMPARE(l.capMinW, 305);
        QCOMPARE(l.capMaxW, 402);
        const Reading r = readReading(only(dir.path()));
        QCOMPARE(r.fanRpm, 0);
        QCOMPARE(r.fanPercent, 0);
    }

    void integratedHasLittle()
    {
        QTemporaryDir dir;
        Fake::cezanne(dir.path());
        const Card c = only(dir.path());
        const Reading r = readReading(c);
        QCOMPARE(r.busy, 4);
        QCOMPARE(r.gttTotal, 8273260544ll);
        QCOMPARE(r.edge, 52.25);
        QCOMPARE(r.fanRpm, -1);
        QCOMPARE(r.fanPercent, -1);
        QCOMPARE(r.mclk, -1);
        const Limits l = readLimits(c);
        QVERIFY(!l.hasFan);
        QVERIFY(!l.hasPowerCap);
        QVERIFY(!l.canCurve());
        QCOMPARE(l.profiles.size(), 7);
        QVERIFY(std::isnan(l.tempCrit));
    }

    void cardWithoutHwmon()
    {
        QTemporaryDir dir;
        const Fake::Tree t = Fake::polaris(dir.path());
        QDir(t.hw).removeRecursively();
        const Card c = only(dir.path());
        QVERIFY(c.hwmon.isEmpty());
        const Reading r = readReading(c);
        QCOMPARE(r.busy, 12);
        QCOMPARE(r.fanRpm, -1);
        const Limits l = readLimits(c);
        QVERIFY(!l.hasFan);
        QVERIFY(!l.hasPowerCap);
        QCOMPARE(l.level, "auto");
    }
};

QTEST_GUILESS_MAIN(TestAmdgpu)
#include "test_amdgpu.moc"

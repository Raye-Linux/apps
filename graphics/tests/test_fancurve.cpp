// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fancurve.h"

#include <QTest>

#include <cmath>

using namespace Raye::Graphics;

class TestFanCurve : public QObject
{
    Q_OBJECT

    const QList<CurvePoint> curve{{40, 20}, {60, 40}, {80, 100}};

private Q_SLOTS:
    void between()
    {
        QCOMPARE(speedAt(curve, 20), 20);
        QCOMPARE(speedAt(curve, 40), 20);
        QCOMPARE(speedAt(curve, 50), 30);
        QCOMPARE(speedAt(curve, 70), 70);
        QCOMPARE(speedAt(curve, 95), 100);
        QCOMPARE(speedAt({}, 50), 100);
        QCOMPARE(speedAt(curve, NAN), 100);
    }

    void pwm()
    {
        QCOMPARE(percentToPwm(0, 0, 255), 0);
        QCOMPARE(percentToPwm(100, 0, 255), 255);
        QCOMPARE(percentToPwm(50, 0, 255), 128);
        QCOMPARE(percentToPwm(150, 0, 255), 255);
        QCOMPARE(percentToPwm(-3, 30, 255), 30);
    }

    void upAtOnceDownSlowly()
    {
        FanController f(curve, 84);
        QCOMPARE(f.step(50), 30);
        QCOMPARE(f.step(70), 70);
        // a degree or two lower: the fan holds
        QCOMPARE(f.step(69), 70);
        QCOMPARE(f.step(68), 70);
        // cooled by three degrees: it follows the curve again
        QCOMPARE(f.step(67), 61);
        QCOMPARE(f.step(72), 76);
    }

    void tooHotOrBlind()
    {
        FanController f(curve, 84);
        QCOMPARE(f.step(45), 25);
        QCOMPARE(f.step(84), 100);
        QCOMPARE(f.step(83), 100);
        FanController g({{40, 0}, {90, 0}}, 84);
        QCOMPARE(g.step(60), 0);
        // no reading counts as too hot, and holds like a hot reading
        QCOMPARE(g.step(NAN), 100);
        QCOMPARE(g.step(83), 100);
        QCOMPARE(g.step(80), 0);
    }
};

QTEST_GUILESS_MAIN(TestFanCurve)
#include "test_fancurve.moc"

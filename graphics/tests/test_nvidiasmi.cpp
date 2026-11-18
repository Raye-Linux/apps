// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nvidiasmi.h"

#include <QTest>

using namespace Raye::Graphics;

class TestNvidiaSmi : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void desktopCard()
    {
        const QList<NvidiaReading> r = parseNvidiaSmi(
            "NVIDIA GeForce RTX 3080, 580.95.05, 00000000:01:00.0, 37, 12, 1710, 9501, 2048, 10240, 61, 45, 212.34, 320.00\n");
        QCOMPARE(r.size(), 1);
        QCOMPARE(r.at(0).name, "NVIDIA GeForce RTX 3080");
        QCOMPARE(r.at(0).driver, "580.95.05");
        QCOMPARE(r.at(0).slot, "0000:01:00.0");
        QCOMPARE(r.at(0).busy, 37);
        QCOMPARE(r.at(0).sclk, 1710);
        QCOMPARE(r.at(0).vramTotal, 10240ll * 1024 * 1024);
        QCOMPARE(r.at(0).temp, 61.0);
        QCOMPARE(r.at(0).fanPercent, 45);
        QCOMPARE(r.at(0).powerW, 212.34);
        QCOMPARE(r.at(0).powerLimitW, 320.0);
    }

    void laptopCardWithoutFanOrPower()
    {
        const QList<NvidiaReading> r = parseNvidiaSmi(
            "NVIDIA GeForce RTX 4060 Laptop GPU, 580.95.05, 00000000:01:00.0, 0, 0, 210, 405, 5, 8188, 41, [N/A], [N/A], [Not Supported]\n"
            "\n");
        QCOMPARE(r.size(), 1);
        QCOMPARE(r.at(0).fanPercent, -1);
        QVERIFY(std::isnan(r.at(0).powerW));
        QVERIFY(std::isnan(r.at(0).powerLimitW));
    }

    void errorsAreNotCards()
    {
        QVERIFY(parseNvidiaSmi("NVIDIA-SMI has failed because it couldn't communicate with the NVIDIA driver.\n").isEmpty());
        QVERIFY(parseNvidiaSmi("").isEmpty());
    }

    void askedForTheRightColumns()
    {
        const QStringList a = nvidiaSmiArguments();
        QCOMPARE(a.size(), 2);
        QCOMPARE(a.at(0).count(','), 12);
        QVERIFY(a.at(1).contains("nounits"));
    }
};

QTEST_GUILESS_MAIN(TestNvidiaSmi)
#include "test_nvidiasmi.moc"

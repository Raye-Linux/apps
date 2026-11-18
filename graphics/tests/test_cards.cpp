// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Finding the cards, against sysfs trees of their own.

#include "cards.h"
#include "fakecards.h"

#include <QTemporaryDir>
#include <QTest>

using namespace Raye::Graphics;

class TestCards : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void hybridLaptop()
    {
        QTemporaryDir dir;
        Fake::intel(dir.path(), 0);
        Fake::navi21(dir.path(), 1);
        const QList<Card> cards = findCards(dir.path(), Fake::pciIds());
        QCOMPARE(cards.size(), 2);
        // the one that can be tuned comes first, connectors are skipped
        QCOMPARE(cards.at(0).name, "card1");
        QCOMPARE(cards.at(0).vendor, Vendor::Amd);
        QVERIFY(cards.at(0).tunable());
        QCOMPARE(cards.at(0).slot, "0000:03:00.0");
        QCOMPARE(cards.at(0).model, "Radeon RX 6800/6800 XT / 6900 XT");
        QVERIFY(cards.at(0).hwmon.endsWith("/hwmon/hwmon3"));
        QCOMPARE(cards.at(1).vendor, Vendor::Intel);
        QCOMPARE(cards.at(1).model, "Iris Xe Graphics");
        QVERIFY(!cards.at(1).tunable());
    }

    void nvidiaBeforeIntel()
    {
        QTemporaryDir dir;
        Fake::intel(dir.path(), 0);
        Fake::nvidia(dir.path(), 1);
        const QList<Card> cards = findCards(dir.path());
        QCOMPARE(cards.size(), 2);
        QCOMPARE(cards.at(0).vendor, Vendor::Nvidia);
        QCOMPARE(cards.at(1).vendor, Vendor::Intel);
    }

    void productNameWins()
    {
        QTemporaryDir dir;
        Fake::navi31(dir.path());
        const QList<Card> cards = findCards(dir.path(), Fake::pciIds());
        QCOMPARE(cards.size(), 1);
        QCOMPARE(cards.at(0).model, "AMD Radeon RX 7900 XTX");
    }

    void nvidiaIsNotTunable()
    {
        QTemporaryDir dir;
        Fake::nvidia(dir.path(), 0);
        const QList<Card> cards = findCards(dir.path(), Fake::pciIds());
        QCOMPARE(cards.size(), 1);
        QCOMPARE(cards.at(0).vendor, Vendor::Nvidia);
        QCOMPARE(cards.at(0).model, "GeForce RTX 3080");
        QVERIFY(!cards.at(0).tunable());
    }

    void amdOnAnotherDriverIsNotTunable()
    {
        QTemporaryDir dir;
        Fake::card(dir.path(), 0, "1002", "67df", "radeon", "0000:01:00.0");
        QVERIFY(!findCards(dir.path()).at(0).tunable());
    }

    void modelLookupStaysInItsVendor()
    {
        // 1638 is a Cezanne under 1002 and something else under 1022
        QCOMPARE(modelName(Fake::pciIds(), "1002", "1638"), "Radeon Vega Series / Radeon Vega Mobile Series");
        QCOMPARE(modelName(Fake::pciIds(), "1022", "1638"), "Something else");
        QCOMPARE(modelName(Fake::pciIds(), "1002", "0b37"), QString());
        QCOMPARE(modelName({}, "1002", "67df"), QString());
    }

    void noCards()
    {
        QTemporaryDir dir;
        QVERIFY(findCards(dir.path()).isEmpty());
        QVERIFY(findCards(dir.path() + "/missing").isEmpty());
    }

    void slots()
    {
        QVERIFY(isSlot("0000:03:00.0"));
        QVERIFY(isSlot("0000:0c:00.7"));
        QVERIFY(!isSlot("0000:0C:00.0"));
        QVERIFY(!isSlot("0000:03:00.8"));
        QVERIFY(!isSlot("../../etc"));
        QVERIFY(!isSlot("0000:03:00.0\n"));
        QVERIFY(!isSlot(""));
    }

    void cardBySlot()
    {
        QTemporaryDir dir;
        Fake::polaris(dir.path(), 0, "0000:01:00.0");
        Fake::navi21(dir.path(), 1, "0000:03:00.0");
        QCOMPARE(cardAt(dir.path(), "0000:03:00.0").name, "card1");
        QCOMPARE(cardAt(dir.path(), "0000:09:00.0").name, QString());
        QCOMPARE(cardAt(dir.path(), "card1").name, QString());
    }
};

QTEST_GUILESS_MAIN(TestCards)
#include "test_cards.moc"
